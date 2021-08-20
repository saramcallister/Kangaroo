#!/usr/bin/env python3
import argparse
import datetime
import pprint
import math
import re
from collections import defaultdict
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.colors as colors
from matplotlib import cm

import os, sys
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

from util import *
from parameters import *

INITIAL_GROUP_SIZE = 1
NUM_LINES_TO_GROUP = 1

GRAPH_END = 120
LOG_LIMIT = 1

MARKERS = ['o', 's', 'X', 'd', 'P', '^']
LINE_STYLES = ['-', '-.', 'dotted', 'dashed']

def get_file_pair(filename):
    data = load_file(filename)
    if not data:
        # print(f'No data: {filename}')
        return None
    cumulative = data[-1]

    writes = get_mbs_mb(cumulative) * SAMPLE_FACTOR * LAYERS
    match = re.search('flashSize(\d+)MB', filename)
    perc = int(match.group(1)) * 1.0 / MAX_FLASH_CAP
    if writes is None:
        return None
    dev_writes = get_dev_wr(writes, perc)

    miss_rate = get_miss_rate(cumulative)
    return (dev_writes, miss_rate)

def get_log_per(filename):
    res = re.search('logPer(\d*(?:\.\d+)?)', filename)
    if not res:
        # print('FAIL: ', filename)
        return 0
    else:
        return float(res.group(1))

def get_threshold(filename):
    res = re.search('threshold(\d+)', filename)
    if not res:
        # print('FAIL: ', filename)
        return 0
    else:
        return float(res.group(1))

def get_ordered_pairs(filenames, unconstrained):
    mem_size_limit = DRAM_CUTOFF_GB * 1024 / (LAYERS * SAMPLE_FACTOR)
    pairs = []
    if 'logPer' in filenames[0]:
        filenames = sorted(filenames,
            key=get_threshold,
        )
    for filename in filenames:
        flash_size = int(re.search('flashSize(\d+)MB', filename).group(1))
        mem_size = int(re.search('memSize(\d+)MB', filename).group(1))
        if 'scaling' in filename:
            scaling = float(re.search('scaling(\d+\.?\d*)', filename).group(1))
            if not math.isclose(scaling, 1): continue
        if flash_size > MAX_FLASH_CAP:
            continue
        if not unconstrained and mem_size_limit < mem_size:
            # if int(mem_size * LAYERS * SAMPLE_FACTOR) <= int((DRAM_CUTOFF_GB + 4) * 1024):
            #     print("Mem Cutoff", filename, mem_size * LAYERS * SAMPLE_FACTOR, DRAM_CUTOFF_GB * 1024)
            continue
        if not unconstrained and 'logPer100' in filename and mem_size < (DRAM_OVERHEAD_BITS) / (AVG_OBJ_SIZE * 8) * flash_size:
            continue
        pair = get_file_pair(filename)
        if pair:
            pairs.append((pair, filename))
    ordered_pairs = sorted(pairs)
    min_mr = float('inf')
    filtered_pairs = []
    for i, ((wr, mr), f_name) in enumerate(ordered_pairs):
        if mr < min_mr:
            min_mr = mr
            print(f"\t{wr}: {mr * 100.}% from {f_name}")
            filtered_pairs.append((wr, mr))
    filtered_pairs.append((GRAPH_END + 20, min_mr))
    return filtered_pairs, ordered_pairs

def group_files(filenames):
    grouped = defaultdict(list)
    for full_pathname in filenames:
        filename = Path(full_pathname).name
        grouped_name = []

        if 'logPer100' in filename:
            capacity = int(re.search('flashSize(\d+)MB', filename).group(1))
            mem_capacity = int(re.search('memSize(\d+)MB', filename).group(1))
            # if mem_capacity * LAYERS * SAMPLE_FACTOR > DRAM_CUTOFF_GB * 1024:
            #     continue
            grouped_name = 'LS'
        if 'log' in filename and not grouped_name:
            grouped_name = 'Kangaroo'
        if not grouped_name:
            grouped_name = 'SA'

        # add path
        if (full_pathname not in grouped[grouped_name]):
            grouped[grouped_name].append(full_pathname)
    return grouped

def main(filenames, savename, unconstrained):
    grouped_filenames = group_files(filenames)

    matplotlib.rcParams.update({'font.size': 20})
    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    sorted_items = sorted(grouped_filenames.items(), reverse=True)

    for i, (label, filenames) in enumerate(sorted_items):
        print(label)
        pairs, all_pairs = get_ordered_pairs(filenames, unconstrained)
        if not pairs:
            continue
        write_rates, miss_rates = zip(*pairs)
        ind = (i - INITIAL_GROUP_SIZE) % NUM_LINES_TO_GROUP
        if i < INITIAL_GROUP_SIZE:
            ind = i
        marker = MARKERS[i % len(MARKERS)]
        linestyle = LINE_STYLES[ind % len(LINE_STYLES)]

        just_data, _ = zip(*all_pairs)
        all_write_rates, all_misses = zip(*just_data)
        # ax.plot(
        #     all_write_rates,
        #     all_misses,
        #     marker=marker,
        #     markersize=5,
        #     linestyle='',
        #     color=color,
        #     alpha=.3,
        #     zorder=1,
        # )

        ax.plot(
            write_rates,
            miss_rates,
            label=label,
            marker=marker,
            markersize=10,
            linestyle=linestyle,
            linewidth=3,
            color=get_color(label),
            zorder=10,
        )



    xlabel = "Avg. Device Write Rate (MB/s)"
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Miss Ratio")
    ax.grid()
    plt.ylim(0, .5)
    plt.xlim(0, GRAPH_END)
    plt.legend()
    plt.tight_layout()
    plt.savefig(savename)
    print(f"Saved to {savename}")


parser = argparse.ArgumentParser()
parser.add_argument('trace', choices=['tw', 'fb', 'tw2'])
parser.add_argument('save_filename')
parser.add_argument('filenames', nargs='*')
parser.add_argument('--unconstrained', '-u', action='store_true')
args = parser.parse_args()

if args.trace == 'fb':
    globals().update(fb_params)
else:
    raise ValueError(f'Unknown trace arguement: {args.trace}')

main(
    args.filenames,
    args.save_filename,
    args.unconstrained,
)
