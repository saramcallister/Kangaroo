# Kangaroo

Welcome! This repository holds all of the simulation and graphing code for "Kangaroo: Caching Billions of Tiny Object on Flash", which is to appear at SOSP 2021. See the Kangaroo Flash Experiments to find code links and detailed run instructions for the on-flash experiment code, which is implemented as another flash cache in Facebook's CacheLib engine.

## Simulation Code 

### Build Instructions

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

For example, if you want to create a Kangaroo simulation experiment using different log percentages
and pre-log random admission percentages run with a Zipf trace (alpha=0.9) you could run:

```
./genConfigs.py kangaroo-zipf --log 1 3 5 7 9 --rrip 3 --zipf 0.9 --readmission 1 --mem-size-MB 5 --flash-size-MB 20 --rotating-kb 256 --multiple-admission-policies --pre-log-random .8 .9 1 --threshold 2
```

For SA, remove the `--log`, `--rrip`, `--threshold`, `--multiple-admission-policies`, and `--rotating-kb` parameters and change `--pre-log-admission` to `--pre-set-admission` for a random admission policy.

For LS, use the `--no-sets` parameter and remove the `--threshold`, `--rrip`, `--multiple-admission-policies` flags.

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

Kangaroo's full on-flash implementation and the Log-structured code evaluated in the paper exist at https://github.com/saramcallister/CacheLib.

To look at the code:

```
git clone https://github.com/saramcallister/CacheLib.git
git checkout artifact-eval-kangaroo # for Kangaroo and SA code
git checkout artifact-eval-log-only # for LS code, need to be on this branch to run this comparison
```

To build the code:

```
./contrib/build.sh -d -j -v
```

To run the main experiments from the paper:

```
# Kangaroo
sudo ./bin/cachebench_bin --json-test-config ../cachelib/cachebench/test_configs/kangaroo_internal/kangaroo_log.json --progress 300 —progress_stats_file kangaroo-exp.out
# SA
sudo ./bin/cachebench_bin --json-test-config ../cachelib/cachebench/test_configs/kangaroo_internal/cachelib.json --progress 300 --progress_stats_file set-associative.out
# LS
sudo ./bin/cachebench_bin --json-test-config ../cachelib/cachebench/test_configs/kangaroo_internal/log_only.json --progress 300 --progress_stats_file log-structured.out   
```

## Graphing

All the graphing scripts are in the `graph-scripts` directory. With the exception of `miss_ratio_kangaroo_params.py` which can create the Kangaroo breakdown graphs and the headline results graph, these scriptsare run using `./{SCRIPT} {WORKLOAD} output-files`. For `miss_ratio_kangaroo_params.py`, the output files or results are specified in the script.

To change the graphing parameters (scaling, dram limits, device write amplification, etc) or add parameters for a new trace, 
you can modify or add `graph-scripts/paramaters`.
