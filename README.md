# Kangaroo

This repository holds all of the simulation and graphing code for "Kangaroo: Caching Billions of Tiny Object on Flash", which is to appear at SOSP 2021. See the Kangaroo Flash Experiments to find code links and detailed run instructions for the on-flash experiment code, which is implemented as another flash cache in Facebook's CacheLib engine and will be fully open-sourced by September 3rd at the latest (see "The CacheLib Caching Engine: Design and Experiences at Scale" in OSDI 2020 for more details on CacheLib).

## Simulation Code 

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
