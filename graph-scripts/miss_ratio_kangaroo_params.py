#!/usr/bin/env python3
import argparse
from collections import namedtuple
from operator import add
import re
from statistics import mean
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from scipy import stats

import os, sys
currentdir = os.path.dirname(os.path.realpath(__file__))
parentdir = os.path.dirname(currentdir)
sys.path.append(parentdir)

import util
from parameters import *

SystemResults = namedtuple('SystemResults', ['fb'])
main_results = {
    'Kangaroo': SystemResults(0.20435062810918816),
    'Set Associative (SA)': SystemResults(0.28969582067242383),
    'Log Structured (LS)': SystemResults(0.4614799445777144),
}

colors = {
    'Kangaroo': 'tab:orange',
    'Set Associative (SA)': (0.0, 0.0, 0.5, 1.0),
    'Log Structured (LS)': (0.0, 0.8333333333333334, 1.0, 1.0),
}

XLIMIT = 105

eviction_policy_files = [
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-no-log-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits1-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-no-log-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits2-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-no-log-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-no-log-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits4-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
]

random_admission_files = [
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-fbTaoSimple-scaling1.out',
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.1-fbTaoSimple-scaling1.out',
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.25-fbTaoSimple-scaling1.out',
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.5-fbTaoSimple-scaling1.out',
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.75-fbTaoSimple-scaling1.out',
    'outputs/2021-04-23-sim-fb-bighash/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
]

quokka_size_files = [
    'outputs/2021-04-28-sim-fb-no-log-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-randomAdmissionPreSet0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer1-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer10-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer2-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer20-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer3-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer30-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer7-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
]

threshold_files = [
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-threshold2-readmit1-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-threshold3-readmit1-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-threshold4-readmit1-fbTaoSimple-scaling1.out',
    'outputs/2021-04-28-sim-fb-kang-params/kangaroo-memSize53MB-slowWarmup-flashSize6666MB-setCapacity4096B-rripMixedPromotion-rripBits3-logPer5-flushSize256-randomAdmissionPreLog0.9-threshold5-readmit1-fbTaoSimple-scaling1.out',
]

def get_file_pair(filename):
    data = util.load_file(filename)
    if not data:
        # print(f'No data: {filename}')
        return None
    cumulative = data[-1]

    writes = util.get_mbs_mb(cumulative) * SAMPLE_FACTOR * LAYERS
    match = re.search('flashSize(\d+)MB', filename)
    perc = int(match.group(1)) * 1.0 / MAX_FLASH_CAP
    if writes is None:
        return None
    dev_writes = get_dev_wr(writes, perc)

    miss_rate = util.get_miss_rate(cumulative)
    return (writes, miss_rate)
    return (dev_writes, miss_rate)

def main_exp(savedir):
    filename = 'main_experiment.pdf'
    matplotlib.rcParams.update({'font.size': 20})

    graph_data = []
    fig, ax = plt.subplots(figsize=(8,5))

    count = np.arange(len(SystemResults._fields))
    width = .1
    labels = []
    refs = []
    for i, (key, result) in enumerate(main_results.items()):
        print(count + width * i + .01 * i)
        p = ax.barh(count + width * i + .01 * i, list(result), width, color=colors[key], zorder=10)
        labels.append(key)
        refs.append(p)


    plt.xlabel('Miss Ratio')
    plt.grid(zorder=1)
    # plt.tick_params(
    #     axis='x',          # changes apply to the x-axis
    #     which='both',      # both major and minor ticks are affected
    #     bottom=False,      # ticks along the bottom edge are off
    #     top=False,         # ticks along the top edge are off
    #     labelbottom=False) # labels along the bottom edge are off
    # plt.legend(refs, labels)
    ax.set_yticks([0, .11, .22])
    ax.set_yticklabels(labels)
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'Main experiment graph saved to {savefile}')

def eviction_policy(savedir):
    filename = 'eviction_policies.pdf'
    matplotlib.rcParams.update({'font.size': 16})

    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    count = np.arange(len(eviction_policy_files))
    width = .2
    spacing = .05
    labels = ['FIFO', '1', '2', '3', '4']
    refs = []
    for i, file in enumerate(eviction_policy_files):
        data = util.load_file(file)
        dev_wr, miss_rate = get_file_pair(file)
        print(labels[i], miss_rate, dev_wr)
        position = width * (i) + spacing * (i)
        p = ax.bar(position, miss_rate, width, zorder=10, color='tab:orange')
        refs.append(p)

    plt.ylabel('Miss Ratio')
    #plt.legend(refs, labels, loc='lower left')
    print(count * width + count + spacing, labels)
    plt.xticks(count * width + count * spacing, labels)
    plt.xlabel('RRIParoo Bits')
    plt.grid(zorder=1)
    #plt.tick_params(axis='x', which='both', bottom=False, labelbottom=False)
    plt.ylim(0, .5)
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'Eviction policy graph saved to {savefile}')

def random_admission_pre_flash(savedir):
    filename = 'random_admission.pdf'
    matplotlib.rcParams.update({'font.size': 16})

    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    count = np.arange(len(random_admission_files))
    width = .2
    spacing = .05
    dats = []
    for i, file in enumerate(random_admission_files):
        # data = util.load_file(file)
        dev_wr, miss_rate = get_file_pair(file)
        match = re.search('randomAdmissionPreSet(\d+\.?\d*)', file)
        if match:
            label = float(match.group(1))
        else:
            label = 1
        print(label, miss_rate, dev_wr)
        dats.append((label, miss_rate, dev_wr))

    dats = sorted(dats)
    labels, miss_rates, write_rates = zip(*dats)
    ax.plot(
        write_rates,
        miss_rates,
        marker='X',
        markersize=10,
        color='tab:orange',
        linewidth=2,
    )

    perc_loc = (10,20)
    for l, mr, wr in dats:
        label = f"{int(l * 100)}%"
        if int(l) == 1:
            pos = (-10, 10)
            label = ''
        elif int(l * 100) == 75:
            pos = (-15, -17)
            perc_loc = (wr-10, mr-.05)
        else:
            pos = (-5, -20)
        plt.annotate(
            label,
            (wr,mr),
            textcoords="offset points",
            xytext=pos,
            ha='center',
        )

    # perc_loc = (1, 0)
    label_loc = (perc_loc[0] - 50, perc_loc[1] - .1)
    # label_loc = (0, 1)
    plt.annotate('Percent admitted to flash', xy=perc_loc, xytext=label_loc,
            arrowprops=dict(facecolor='black', shrink=0.05))
    print(label_loc, perc_loc)

    ax.set_ylabel('Miss Ratio')
    ax.set_ylim(0, .5)
    ax.set_xlim(0, XLIMIT)
    ax.set_xlabel('Write Rate (MB/s)')
    plt.grid()
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'Random admission graph saved to {savefile}')

def quokka(savedir):
    filename = 'quokka_sizes.pdf'
    matplotlib.rcParams.update({'font.size': 16})

    graph_data = []
    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    count = np.arange(len(quokka_size_files))
    width = .2
    spacing = .05
    dats = []
    for i, file in enumerate(quokka_size_files):
        dev_wr, miss_rate = get_file_pair(file)
        match = re.search('logPer(\d+)', file)
        if match:
            label = int(match.group(1))
        else:
            label = 0
        print(label, miss_rate, dev_wr)
        dats.append((label, miss_rate, dev_wr))

    dats = sorted(dats)
    labels, miss_rates, write_rates = zip(*dats)
    ax.plot(
        write_rates,
        miss_rates,
        marker='X',
        markersize=10,
        linewidth=2,
        color='tab:orange',
    )

    for i, (l, mr, wr) in enumerate(dats):
        label = f"{int(l)}%"
        if i % 2:
            # label=''
            pos = (0, -20)
        else:
            pos = (10, 10)
            perc_loc = (wr, mr - .05)
        plt.annotate(
            label,
            (wr,mr),
            textcoords="offset points",
            xytext=pos,
            ha='center',
        )

    label_loc = (perc_loc[0], perc_loc[1] - .1)
    plt.annotate('KLog Percent', xy=perc_loc, xytext=label_loc,
            arrowprops=dict(facecolor='black', shrink=0.05))
    print(label_loc, perc_loc)

    ax.set_ylabel('Miss Ratio')
    ax.set_xlabel('Write Rate (MB/s)')
    ax.set_ylim(0, .5)
    ax.set_xlim(0, XLIMIT)
    plt.grid()
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'KLog sizes graph saved to {savefile}')

def threshold(savedir):
    filename = 'threshold.pdf'
    matplotlib.rcParams.update({'font.size': 16})

    graph_data = []
    fig, ax = plt.subplots(figsize=GRAPH_SIZE)

    count = np.arange(len(threshold_files))
    width = .2
    spacing = .05
    dats = []
    for i, file in enumerate(threshold_files):
        dev_wr, miss_rate = get_file_pair(file)
        match = re.search('threshold(\d+)', file)
        if match:
            label = int(match.group(1))
        else:
            label = 1
        print(label, miss_rate, dev_wr)
        dats.append((label, miss_rate, dev_wr))

    dats = sorted(dats)
    labels, miss_rates, write_rates = zip(*dats)
    ax.plot(
        write_rates,
        miss_rates,
        marker='X',
        markersize=10,
        linewidth=2,
        color='tab:orange',
    )

    for i, (l, mr, wr) in enumerate(dats):
        label = f"{int(l)}"
        pos = (-10, -20)
        if (i == 0):
            perc_loc = (wr, mr - .05)
        plt.annotate(
            label,
            (wr,mr),
            textcoords="offset points",
            xytext=pos,
            ha='center',
        )

    label_loc = (perc_loc[0] + 20, perc_loc[1] - .1)
    plt.annotate('Threshold', xy=perc_loc, xytext=label_loc,
            arrowprops=dict(facecolor='black', shrink=0.05))
    print(label_loc, perc_loc)

    ax.set_ylabel('Miss Ratio')
    ax.set_xlabel('Write Rate (MB/s)')
    ax.set_ylim(0, .5)
    ax.set_xlim(0, XLIMIT)
    plt.grid()
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'Threshold graph saved to {savefile}')

def alwa_threhold(savedir):
    filename = 'alwa_threshold.pdf'
    matplotlib.rcParams.update({'font.size': 16})

    fig, ax = plt.subplots(figsize=(8, 3))

    count = np.arange(len(random_admission_files))
    width = .2
    spacing = .05
    dats = []
    x = [.25 * poss for poss in range(4, 40)]
    y = [4096 / (threshold * 100) for threshold in x]

    ax.plot(
        x,
        y,
        # marker='X',
        # markersize=10,
        color='black',
        linewidth=2,
    )

    ax.set_ylabel('ALWA')
    ax.set_xlabel('Number of New Items in Set Rewrite')
    ax.set_ylim(0)
    ax.set_xlim(0, 10)
    ax.set_yticks([10, 20, 30, 40])
    plt.grid()
    plt.tight_layout()

    savefile = Path(savedir) / filename
    plt.savefig(str(savefile))
    print(f'Random admission graph saved to {savefile}')

globals().update(fb_params)
if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('save_directory')
    parser.add_argument('-m', '--main-exp', action='store_true')
    parser.add_argument('-e', '--eviction-policy', action='store_true')
    parser.add_argument('-r', '--random-admission', action='store_true')
    parser.add_argument('-q', '--quokka', action='store_true')
    parser.add_argument('-t', '--threshold', action='store_true')
    parser.add_argument('-a', '--alwa-threshold', action='store_true')
    args = parser.parse_args()

    savedir = args.save_directory
    if args.main_exp:
        main_exp(savedir)
    if args.eviction_policy:
        eviction_policy(savedir)
    if args.random_admission:
        random_admission_pre_flash(savedir)
    if args.quokka:
        quokka(savedir)
    if args.threshold:
        threshold(savedir)
    if args.alwa_threshold:
        alwa_threhold(savedir)
