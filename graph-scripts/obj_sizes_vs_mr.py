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

GRAPH_END = 2
LOG_LIMIT = 2

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

def get_file_pair(filename, scaling):
    # print(filename)
    data = load_file(filename)
    if not data or len(data) == 1:
        return None
    cumulative = data[-1]
    writes = get_mbs_mb(cumulative) * SAMPLE_FACTOR * LAYERS
    miss_rate = get_miss_rate(cumulative)
    match = re.search('flashSize(\d+)MB', filename)
    perc = int(match.group(1)) * 1.0 / (MAX_FLASH_CAP * scaling)
    if perc > 1:
        return None
    return (get_dev_wr(writes, perc), miss_rate)

def get_scaling_mr(filenames):
    l = [] # (scaling, miss ratio, filename)
    rejection_criteria = defaultdict(int)
    for filename in filenames:
        capacity = float(re.search('flashSize(\d+)MB', filename).group(1))
        mem_capacity = float(re.search('memSize(\d+)MB', filename).group(1))
        scaling = 1.0
        if 'scaling' in filename:
            scaling = float(re.search('scaling(\d+\.?\d*)', filename).group(1))
        pair = get_file_pair(filename, scaling)
        if not pair:
            continue
        if scaling > 1.9:
            continue
        if pair[0] > WRITE_RATE_LIMIT * scaling:
            rejection_criteria[f'{scaling}_write_rate'] += 1
            continue
        if capacity > MAX_FLASH_CAP * scaling:
            rejection_criteria[f'{scaling}_flash_cap'] += 1
            continue

        if 'logPer100' in filename and mem_capacity < (DRAM_OVERHEAD_BITS)/(AVG_OBJ_SIZE * 8) * capacity:
            rejection_criteria[f'{scaling}_log_overhead'] += 1
            # print((DRAM_OVERHEAD_BITS)/(AVG_OBJ_SIZE * 8) * capacity, mem_capacity, capacity)
            continue
        mem_size_limit = scaling * DRAM_CUTOFF_GB * 1024 / (LAYERS * SAMPLE_FACTOR)
        # print(mem_size_limit, mem_capacity)
        if mem_capacity > mem_size_limit:
            rejection_criteria[f'{scaling}_mem_limit'] += 1
            continue
        l.append((scaling, pair[1], filename))
    print(rejection_criteria)

    d = {}
    for scaling, mr, filename in l:
        if scaling not in d or mr < d[scaling][0]:
            d[scaling] = (mr, filename)
    ret = []
    for scaling, (mr, f_name) in sorted(d.items()):
        print(f"\t{scaling}: {mr * 100.}% from {f_name}")
        ret.append((scaling, mr))
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

def main(filenames, savename):
    grouped_filenames = group_files(filenames)

    matplotlib.rcParams.update({'font.size': 20})
    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    sorted_items = sorted(grouped_filenames.items(), reverse=True)

    for i, (label, filenames) in enumerate(sorted_items):
        print(label)
        pairs, all_pairs = get_scaling_mr(filenames)
        if not pairs:
            continue
        cap, miss_rates = zip(*pairs)
        ind = (i - INITIAL_GROUP_SIZE) % NUM_LINES_TO_GROUP
        if i < INITIAL_GROUP_SIZE:
            ind = i
        marker = MARKERS[i % len(MARKERS)]
        linestyle = LINE_STYLES[ind % len(LINE_STYLES)]
        if label == 'Baseline, Sets Only':
            marker='*'

        # all_dram, all_mr, files = zip(*all_pairs)
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

        x = [scaling * AVG_OBJ_SIZE for scaling in cap]
        ax.plot(
            x,
            miss_rates,
            label=label,
            marker=marker,
            markersize=10,
            linestyle=linestyle,
            linewidth=3,
            color=get_color(label),
            zorder=10,
        )

    xlabel = "Average Object Size"
    ax.set_xlabel(xlabel)
    ax.set_ylabel("Miss Ratio")
    ax.grid()
    plt.ylim(0, .5)
    plt.xlim(0)
    plt.legend()
    plt.tight_layout()
    plt.savefig(savename)
    print(f"Saved to {savename}")


parser = argparse.ArgumentParser()
parser.add_argument('trace', choices=['tw', 'fb', 'tw2'])
parser.add_argument('save_filename')
parser.add_argument('dirs', nargs='*')
args = parser.parse_args()

if args.trace == 'fb':
    globals().update(fb_params)
else:
    raise ValueError(f'Unknown trace arguement: {args.trace}')

all_filenames = []
for dir in args.dirs:
    p = Path(dir)
    all_filenames.extend(str(x) for x in p.iterdir() if x.name[0] != '.')

main(
    all_filenames,
    args.save_filename
)
