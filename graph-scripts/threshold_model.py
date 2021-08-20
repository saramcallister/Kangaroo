#!/usr/bin/env python3

from scipy.stats import binom
import math
import argparse
from collections import defaultdict
from pathlib import Path

import matplotlib
import matplotlib.pyplot as plt

def objects_to_bin_model(log_items, num_sets, bin_size, threshold, max_x_returned, max_retries=0):
    assert(threshold >= 1)

    if log_items == 0:
        return bin_size

    avg_num_objects_in_log = log_items
    rv = binom(avg_num_objects_in_log, 1.0/num_sets)
    x = list(range(0, max_x_returned))
    pmf = rv.pmf(x)
    # print(sum([x * (i + threshold) for i, x in enumerate(pmf[threshold:])])/ (1 - sum(pmf[0:threshold])))
    # print(rv.expect())
    # print(threshold)
    # print(rv.expect(lb=threshold, conditional=True))

    # limit binomial distribution according to threshold
    p_below_threshold = min(sum(pmf[0:threshold]), 1)
    p = 1 - p_below_threshold
    print(threshold, ": ", p_below_threshold - pmf[0])
    # p_below_threshold = .60
    if threshold == 1:
        p_below_threshold = 0


    logging_cost = 1

    y = pmf[threshold:]/(1 - p_below_threshold)
    write_amp = logging_cost + bin_size*(1-p_below_threshold)/rv.expect(lb=threshold, conditional=True)
    return write_amp, (p) / (1 - pmf[0])

def main(filename):
    f = 1
    q = 500000000 * 2#* 4
    s = 463867188 #* 2
    w = 40
    # f = .92 # scale of items in log due fragmentation or inaccurate index
    # q = 500000000 / 2# items in log
    # s = 463867188  / 2# num sets
    # w = 20 #32 # items per set
    max_ret = 50 # how far into future to explore

    max_threshold = 5

    total_objs = q + s*w

    matplotlib.rcParams.update({'font.size': 16})

    fig, ax = plt.subplots(figsize=(4.5, 3))

    probs = defaultdict(list)

    for i, obj_size in enumerate([50, 100, 200, 500]):
        was = []
        print(f'\nObj Size: {obj_size}')
        print("log per", (2*q)/(2*q + s*w))
        # for log_size in [1/5 * q, 2/5 * q, q, 2*q, 4*q]:
        for thres in range(1,max_threshold):
        #for log_size in [67647, 207125, 352477, 744117]:
            #print(f'Log Size {log_size_mb} MB, {bytes_log_size/ssd_size * 100}%')
            if thres * obj_size > 4096:
                continue
            write_amp, admission_prob = objects_to_bin_model(q / (obj_size / 100), s, 4096 / obj_size, thres, max_ret)
            was.append(write_amp)
            probs[obj_size].append(100 * admission_prob)
            print('\tWrite Amp: ', write_amp)
        # ax.plot([1, 2, 5, 10, 20], was, linewidth=2, label=f'{obj_size} B', marker='x', zorder=20-i)
        ax.plot(range(1, len(was) + 1), was, linewidth=2, label=f'{obj_size} B', marker='x', zorder=20-i)

    plt.grid()
    plt.legend()
    plt.xlabel('Threshold')
    plt.ylabel('ALWA')
    plt.tight_layout()
    plt.savefig(filename)
    print(f'Saved to {filename}')

    filename = Path(filename).parent / 'admission-perc.pdf'
    fig, ax = plt.subplots(figsize=(4.5, 3))

    for obj_size, prob_list in probs.items():
        print(f'\t{obj_size}: {prob_list}')
        # ax.plot([1, 2, 5, 10, 20], prob_list, linewidth=2, label=f'{obj_size} B', marker='x', zorder=20)
        ax.plot(range(1, len(was) + 1), prob_list, linewidth=2, label=f'{obj_size} B', marker='x', zorder=20)

    plt.grid()
    # plt.legend()
    plt.xlabel('Threshold')
    plt.ylabel('% Admitted')
    plt.tight_layout()
    plt.savefig(str(filename))
    print(f'Saved to {filename}')

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('save_filename')
    args = parser.parse_args()
    main(args.save_filename)
