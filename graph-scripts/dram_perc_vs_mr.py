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

GRAPH_END_PERC = 3.3
GRAPH_END = 40
LOG_LIMIT = 2

MARKERS = ['o', 's', 'X', 'd', 'P', '^']
LINE_STYLES = ['-', '-.', 'dotted', 'dashed']

def get_file_pair(filename):
    data = load_file(filename)
    if not data:
        return None
    cumulative = data[-1]
    writes = get_mbs_mb(cumulative) * SAMPLE_FACTOR * LAYERS
    miss_rate = get_miss_rate(cumulative)
    # return writes, miss_rate
    match = re.search('flashSize(\d+)MB', filename)
    perc = int(match.group(1)) * 1.0 / MAX_FLASH_CAP
    return (get_dev_wr(writes, perc), miss_rate)

def get_dram_perc_mr(filenames, unconstrained):
    l = [] # (dram_perc, miss ratio, filename)
    for filename in filenames:
        capacity = float(re.search('flashSize(\d+)MB', filename).group(1))
        mem_capacity = float(re.search('memSize(\d+)MB', filename).group(1))
        pair = get_file_pair(filename)

        if capacity > MAX_FLASH_CAP:
            continue
        if 'scaling' in filename:
            scaling = float(re.search('scaling(\d+\.?\d*)', filename).group(1))
            if not math.isclose(scaling, 1): continue

        if pair and unconstrained:
            l.append((mem_capacity, pair[1], filename))
        elif pair and pair[0] < WRITE_RATE_LIMIT and capacity <= MAX_FLASH_CAP:
            l.append((mem_capacity, pair[1], filename))
        elif 'large-log' in filename and 'logPer100' not in filename:
            print(filename, pair[0], WRITE_RATE_LIMIT, capacity, MAX_FLASH_CAP, pair[1])

    d = {}
    for dram, mr, filename in l:
        if dram not in d or mr < d[dram][0]:
            d[dram] = (mr, filename)
    ret = []
    min_mr = float('inf')
    for cap, (mr, f_name) in sorted(d.items()):
        if mr <= min_mr:
            ret.append((cap, mr))
            print(f"\t{cap}: {mr * 100.}% from {f_name}")
            min_mr = min(mr, min_mr)
    ret.append((GRAPH_END * LAYERS * SAMPLE_FACTOR, min_mr))
    return ret, sorted(l)

def group_files(filenames):
    grouped = defaultdict(list)
    for full_pathname in filenames:
        filename = Path(full_pathname).name
        grouped_name = []

        if 'logPer100' in filename:
            grouped_name = 'LS'
        if 'log' in filename and not grouped_name:
            grouped_name = 'Kangaroo'
        if not grouped_name:
            grouped_name = 'SA'

        # add path
        if (full_pathname not in grouped[grouped_name]):
            grouped[grouped_name].append(full_pathname)

    return grouped

def main(filenames, savename, unconstrained, perc):
    grouped_filenames = group_files(filenames)

    matplotlib.rcParams.update({'font.size': 20})
    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    sorted_items = sorted(grouped_filenames.items(), reverse=True)

    for i, (label, filenames) in enumerate(sorted_items):
        print(label)
        pairs, all_pairs = get_dram_perc_mr(filenames, unconstrained)
        if not pairs:
            continue
        cap, miss_rates = zip(*pairs)
        if perc:
            cap = [100. * mem_capacity / MAX_FLASH_CAP for mem_capacity in cap]
        else:
            cap = [mem_capacity * SAMPLE_FACTOR * LAYERS / 1024 for mem_capacity in cap]
        ind = (i - INITIAL_GROUP_SIZE) % NUM_LINES_TO_GROUP
        if i < INITIAL_GROUP_SIZE:
            ind = i
        marker = MARKERS[i % len(MARKERS)]
        linestyle = LINE_STYLES[ind % len(LINE_STYLES)]
        if label == 'Baseline, Sets Only':
            marker='*'

        # all_dram, all_mr, files = zip(*all_pairs)
        # if perc:
        #     all_dram = [100. * mem_capacity / MAX_FLASH_CAP for mem_capacity in all_dram]
        # else:
        #     all_dram = [mem_capacity * SAMPLE_FACTOR * LAYERS / 1024 for mem_capacity in all_dram]
        # ax.plot(
        #     all_dram,
        #     all_mr,
        #     marker=marker,
        #     markersize=5,
        #     linestyle='',
        #     color=color,
        #     alpha=.3,
        #     # label='',
        #     zorder=1,
        # )

        ax.plot(
            cap,
            miss_rates,
            label=label,
            marker=marker,
            markersize=10,
            linestyle=linestyle,
            linewidth=3,
            color=get_color(label),
            zorder=10,
        )
        # print(label, color)

    if perc:
        xlabel = "% DRAM "
    else:
        xlabel = "DRAM (GB)"
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Miss Ratio")
    ax.grid()
    plt.ylim(0, .5)
    if perc:
        plt.xlim(0, GRAPH_END_PERC)
    else:
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
parser.add_argument('--perc', '-p', action='store_true')
args = parser.parse_args()

if args.trace == 'fb':
    globals().update(fb_params)
else:
    raise ValueError(f'Unknown trace arguement: {args.trace}')

main(
    args.filenames,
    args.save_filename,
    args.unconstrained,
    args.perc,
)
