#include <cmath>

#include "caches/mem_log_cache.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "lru.hpp"
#include "stats/stats.hpp"

namespace cache {

MemLogCache::MemLogCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings):
    Cache(sc, gs, settings) {
    misc::ConfigReader cfg(settings);
    
    /* Initialize flash cache, split size among log and 
     * sets so that sets are always a multiple of setCapacity
     * and close to provided percentLog, leans toward more log */
    uint64_t flash_size_mb = (uint64_t) cfg.read<int>("cache.flashSizeMB");
    uint64_t log_capacity = flash_size_mb * 1024 * 1024;

    /* figure out what log version to use and initialize it */
    uint64_t readmit = cfg.read<int>("log.readmit", 0);
    auto& log_stats = statsCollector->createLocalCollector("log");
    uint64_t block_size = 1024 * (uint64_t) cfg.read<int>("log.flushBlockSizeKB", 256);
    _log = new flashCache::LogOnly(log_capacity, block_size, log_stats, readmit);

    uint64_t memory_size = (uint64_t) cfg.read<int>("cache.memorySizeMB") * 1024 * 1024;
    auto& memory_cache_stats = statsCollector->createLocalCollector("memCache");
    double perc_mem_log_overhead = cfg.read<float>("cache.memOverheadRatio", INDEX_LOG_RATIO);
    std::cout << "Log Capacity: " << log_capacity << " mem capacity " << memory_size 
        << " percent " << perc_mem_log_overhead
        << std::endl;
    assert(log_capacity * perc_mem_log_overhead <= memory_size);
    uint64_t mem_cache_capacity = memory_size - (log_capacity * perc_mem_log_overhead);
    std::cout << "Actual Memory Cache Size after indexing costs: " 
        << mem_cache_capacity << std::endl;
    _memCache = new memcache::LRU(mem_cache_capacity, memory_cache_stats);
    
    /* Initialize prelog admission policy */
    if (cfg.exists("preLogAdmission")) {
        std::string policyType = cfg.read<const char*>("preLogAdmission.policy");
        std::cout << "Creating admission policy of type " << policyType << std::endl;
        policyType.append(".preLogAdmission");
        const libconfig::Setting& admission_settings = cfg.read<libconfig::Setting&>("preLogAdmission");
        auto& admission_stats = statsCollector->createLocalCollector(policyType);
        _prelog_admission = admission::Policy::create(admission_settings, nullptr, _log, admission_stats);
    }
    
    /* slow warmup */
    if (cfg.exists("cache.slowWarmup")) {
        warmed_up = true;
    }
    assert(warmed_up);
}

MemLogCache::~MemLogCache() {
    delete _log;
    delete _memCache;
    delete _prelog_admission;
}
  

void MemLogCache::insert(candidate_t id) {
    std::vector<candidate_t> ret = _memCache->insert(id);
    if (warmed_up && _prelog_admission) {
        ret = _prelog_admission->admit_simple(ret);
    } 
    ret = _log->insert(ret);

    /* check warmed up condition every so often */
    if (!warmed_up && getAccessesAfterFlush() % CHECK_WARMUP_INTERVAL == 0) {
        checkWarmup();
    }
}

bool MemLogCache::find(candidate_t id) {
    if (_memCache->find(id) || _log->find(id)) {
        return true;
    }
    return false;
}

double MemLogCache::calcFlashWriteAmp() {
    double flash_write_amp = _log->calcWriteAmp(); 
    if (warmed_up && _prelog_admission) {
        /* include all bytes not written to structs */
        return flash_write_amp * _prelog_admission->byteRatioAdmitted();
    } else {
        return flash_write_amp;
    }
}

void MemLogCache::checkWarmup() {
    if (_log->ratioEvictedToCapacity() < 1) {
        return;
    }
    flushStats();
    std::cout << "Reached end of warmup, resetting stats\n\n";
    _log->flushStats();
    _memCache->flushStats();
    dumpStats();
    warmed_up = true;
}

} // namespace cache
