#!/usr/bin/python3

import glob, subprocess, sys, argparse, multiprocessing, os

parser = argparse.ArgumentParser()
parser.add_argument("dirs", nargs="+", help="Experiment directories")
parser.add_argument("--jobs", default=1, type=int)
args = parser.parse_args()

def runExperiment(cfg):
    print(f'Running {cfg}...')

    expName = os.path.splitext(cfg)[0]
    
    cmd = f'../simulator/bin/cache {cfg}'
    print (cmd)

    if subprocess.run(cmd, shell=True).returncode == 0:
        print(f'{cfg} finished successfully')
    else:
        print(f'Error running {cfg}!')
                
if __name__ == "__main__":
    dirs = args.dirs

    configs = []
    
    for d in dirs:
        expDir = f'{d}'
        print('Running experiments in %s...' % expDir)
        configs += glob.glob(expDir + '/*.cfg')
                
    with multiprocessing.Pool(args.jobs) as pool:
        pool.map(runExperiment, configs)

    print('Done.')
