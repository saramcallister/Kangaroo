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

GRAPH_END = 3200

DEVICE_SIZES = [256, 512, 1024, 1536, 2048, 2560, 3072] # gb

MARKERS = ['o', 's', 'X', 'd', 'P', '^']
LINE_STYLES = ['-', '-.', 'dotted', 'dashed']

def get_file_pairs(filename):
    data = load_file(filename)
    if not data:
        return []
    cumulative = data[-1]
    writes = get_mbs_mb(cumulative) * SAMPLE_FACTOR * LAYERS
    miss_rate = get_miss_rate(cumulative)
    flash_size = int(re.search('flashSize(\d+)MB', filename).group(1))

    ret = []
    for dev_size in DEVICE_SIZES:
        if dev_size * 1024 >= (flash_size * SAMPLE_FACTOR * LAYERS):
            perc = flash_size  * SAMPLE_FACTOR * LAYERS / (dev_size * 1024)
            dev_wr = get_dev_wr(writes, perc)
            ret.append((dev_size, dev_wr, miss_rate))
    return ret

def get_flash_cap_mr(filenames, unconstrained):
    mem_size_limit = DRAM_CUTOFF_GB * 1024 / (LAYERS * SAMPLE_FACTOR)
    l = [] # (capacity, miss ratio, filename)
    for filename in filenames:
        flash_used = int(re.search('flashSize(\d+)MB', filename).group(1))
        mem_size = int(re.search('memSize(\d+)MB', filename).group(1))
        if 'scaling' in filename:
            scaling = float(re.search('scaling(\d+\.?\d*)', filename).group(1))
            if not math.isclose(scaling, 1): continue
        pairs = get_file_pairs(filename)
        for dev_size, dev_wr, mr in pairs:
            write_rate_limit = DWPD * dev_size * 1024 / (60 * 60 * 24)
            # print(dev_size, dev_wr, mr, filename)
            if unconstrained:
                l.append((dev_size, mr, filename))
            elif dev_wr <= write_rate_limit and mem_size <= mem_size_limit:
                if 'logPer100' in filename and mem_size < (DRAM_OVERHEAD_BITS) / (AVG_OBJ_SIZE * 8) * flash_used:
                    continue
                l.append((dev_size, mr, filename))
    d = {}
    for cap, mr, filename in l:
        if cap not in d or mr < d[cap][0]:
            d[cap] = (mr, filename)
    ret = []
    min_mr = float('inf')
    for cap, (mr, f_name) in sorted(d.items()):
        if min_mr >= mr:
            ret.append((cap, mr))
            print(f"\t{cap}: {mr * 100.}% from {f_name}")
            min_mr = min(mr, min_mr)
    ret.append((GRAPH_END * 2, min_mr))
    return ret, sorted(l)

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
        pairs, all_pairs = get_flash_cap_mr(filenames, unconstrained)
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

        # if not all_pairs:
        #     continue
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

    xlabel = "Flash Device Capacity (GB)"
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
