#pragma once

#include <stdint.h>

namespace cache {

const int STATS_INTERVAL_POWER = 6;
const uint64_t CHECK_WARMUP_INTERVAL = 1000;
const double INDEX_LOG_RATIO = 0.02;
const uint64_t SIZE_BUCKETING = 10;

}

namespace parser{

const uint64_t FAST_FORWARD = 0;

}

namespace flashCache {

const int HIT_BIT_VECTOR_SIZE = 32;
const uint32_t MIN_FLASH_WRITE_BYTES = 4096;
const uint64_t EVICT_SET_LIMIT = 16000; // high enough to not really effect results
const int RRIP_LONG_DIFF = 1; // less than distant
const int RRIP_DISTANT_DIFF = 1;
const double AVG_OBJ_SIZE_BYTES = 330; // just for memory estimate

}

