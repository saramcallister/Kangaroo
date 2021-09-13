# Kangaroo

Welcome! This repository holds all of the simulation and graphing code for "Kangaroo: Caching Billions of Tiny Object on Flash", which is to appear at SOSP 2021. See the Kangaroo Flash Experiments to find code links and detailed run instructions for the on-flash experiment code, which is implemented as another flash cache in Facebook's CacheLib engine.

## Simulation Code 

#### Build Instructions

Install SCons (https://scons.org/). Recommended install using python:

```
pip install SCons
cd simulator
scons
```

The executable is `simulator/bin/cache` which can be run with individual configurations.

#### Generate Simulator Configurations

The easiest way to generate workloads is using the `genConfigs.py` script in `run-scripts`.

```
cd run-scripts
./genConfigs.py -h # to see all the options
```

For example, if you want to create a Kangaroo simulation experiment using different log percentages (1%, 3%, 5%, 7%, 9% of flash)
and pre-log random admission percentages (admit 80%, 90%, and 100%) run with a Zipf trace (alpha=0.9) you could run:

```
./genConfigs.py kangaroo-zipf --log 1 3 5 7 9 --rrip 3 --zipf 0.9 --readmission 1 --mem-size-MB 5 --flash-size-MB 20 --rotating-kb 256 --multiple-admission-policies --pre-log-random .8 .9 1 --threshold 2
```

For SA, remove the `--log`, `--rrip`, `--threshold`, `--multiple-admission-policies`, `readmission`, and `--rotating-kb` parameters and change `--pre-log-random` to `--pre-set-random` for a random admission policy.

```
./genConfigs.py sa-zipf --zipf 0.9 --mem-size-MB 5 --flash-size-MB 20 --pre-set-random .8 .9 1
```

For LS, use the `--no-sets` parameter and remove the `--threshold`, `--rrip`, `--multiple-admission-policies` flags.

```
./genConfigs.py ls-zipf --zipf 0.9 --mem-size-MB 5 --flash-size-MB 20 --pre-log-random .8 .9 1 --no-sets
```

The configs will automatically be generated in local `run-scripts/configs` directory with outputs to be written to `run-scripts/output`.

Note: Python 3 is required to run python code.

#### Run Simulator

To run the generated configurations, use `run-scripts/runLocal.py`.

```
./runLocal.py configs --jobs 3
```

Each simulator instance is single threaded. Multiple configs can be run at once using the jobs parameter.

#### Adding Parsing Code

One of the most common code additions is adding parsers for new trace formats. To add a new parser,
1) Write a parser that inherits from Parser (in `simulator/parsers/parser.hpp`). Examples can be found in the `parsers` directory.
2) Add the parser as an option in `simulator/parsers/parser.cpp`.
3) Add parser to the configuration generator if desired (`run-scripts/genConfigs.py`).

## Kangaroo Flash Experiments

Kangaroo's full on-flash implementation and the Log-structured code evaluated in the paper exist at https://github.com/saramcallister/CacheLib-1

To look at the Kangaroo code:
```
git clone git@github.com:saramcallister/CacheLib-1.git
cd CacheLib-1

git checkout artifact-eval-kangaroo-upstream # for Kangaroo and SA code
git checkout artifact-eval-log-only-upstream # for LS code, need to be on this branch to run this comparison 
```
The Kangaroo flash cache code is mostly contained in `cachelib/navy/kangaroo`. 
You can also look at the pull request to the main [CacheLib repository](https://github.com/facebook/CacheLib).

To build the code:

```
./contrib/build.sh -j -v -d
```

To run the flash experiments, you will need a flash drive formatted as a raw block device. The Cachelib engine has support for reading device-level write amplification, but it might not support your specific device. You can still get application-level write amplification numbers.

To run an experiment, create an appropriate json config (examples below) and run:
```
sudo ./opt/cachelib/bin/cachebench --json-test-config {CONFIG} --progress 300 --progress_stats_file {OUTFILE}
```
This will write experiment statistics from CacheLib to the specified output file every 300 seconds. 

Sample configuartions are at `cachelib/cachebench/test_configs/kangaroo`
within the CacheLib-Kangaroo fork. In the directory, there is also a text file that explains important parameters (more general parameters are available at https://cachelib.org/).
The appropriate configurations for *Kangaroo* and *LS* are only in the correct branch.

## Graphing

All the graphing scripts are in the `graph-scripts` directory. With the exception of `miss_ratio_kangaroo_params.py` which can create the Kangaroo breakdown graphs and the headline results graph, these scriptsare run using `./{SCRIPT} {WORKLOAD} output-files`. For `miss_ratio_kangaroo_params.py`, the output files or results are specified in the script.

To change the graphing parameters (scaling, dram limits, device write amplification, etc) or add parameters for a new trace, 
you can modify or add `graph-scripts/paramaters`.
