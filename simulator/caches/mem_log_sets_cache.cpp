#include <cmath>

#include "caches/mem_log_sets_cache.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "log.hpp"
#include "lru.hpp"
#include "rotating_log.hpp"
#include "rrip_sets.hpp"
#include "sets.hpp"
#include "stats/stats.hpp"

namespace cache {

MemLogSetsCache::MemLogSetsCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings):
    Cache(sc, gs, settings) {
    misc::ConfigReader cfg(settings);
    
    /* Initialize flash cache, split size among log and 
     * sets so that sets are always a multiple of setCapacity
     * and close to provided percentLog, leans toward more log */
    uint64_t flash_size_mb = (uint64_t) cfg.read<int>("cache.flashSizeMB");
    double log_percent = (double) cfg.read<float>("log.percentLog") / 100.;
    uint64_t set_capacity = (uint64_t) cfg.read<int>("sets.setCapacity");
    uint64_t flash_size = flash_size_mb * 1024 * 1024;
    double exact_set_capacity = flash_size * (1 - log_percent);
    uint64_t actual_set_capacity = exact_set_capacity - std::fmod(exact_set_capacity, set_capacity);
    uint64_t log_capacity = flash_size - actual_set_capacity;
    if (cfg.exists("log.adjustFlashSizeUp")) {
        actual_set_capacity += (log_capacity / 2);
    }
    std::cout << "Desired Percent Log: " << log_percent
        << " Actual Percent Log: " << log_capacity/ (double) flash_size
        << std::endl;
    
    auto& set_stats = statsCollector->createLocalCollector("sets");
    uint64_t num_sets = actual_set_capacity / set_capacity;
    int num_hash_functions = cfg.read<int>("sets.numHashFunctions", 1);
    if (cfg.exists("sets.rripBits")) {
        int rrip_bits = cfg.read<int>("sets.rripBits");
        bool promotion = (bool) cfg.read<int>("sets.promotionOnly", 0);
        bool mixed = (bool) cfg.read<int>("sets.mixedRRIP", 0);
        _sets = new flashCache::RripSets(num_sets, set_capacity, set_stats, this, 
                num_hash_functions, rrip_bits, promotion, mixed);
    } else {
        bool sets_track_hits = cfg.exists("sets.trackHitsPerItem");
        _sets = new flashCache::Sets(num_sets, set_capacity, set_stats, this, num_hash_functions, sets_track_hits);
    }
    if (cfg.exists("sets.hitDistribution")) {
        std::cout << "CALLED enable set hits distribution" << std::endl;
        _sets->enableHitDistributionOverSets();
    }

    /* figure out what log version to use and initialize it */
    uint64_t readmit = cfg.read<int>("log.readmit", 0);
    if (cfg.exists("log.flushBlockSizeKB")) {
        auto& log_stats = statsCollector->createLocalCollector("log");
        uint64_t block_size = 1024 * (uint64_t) cfg.read<int>("log.flushBlockSizeKB");
        _log = new flashCache::RotatingLog(log_capacity, block_size, _sets, log_stats, readmit);
    } else {
        auto& log_stats = statsCollector->createLocalCollector("log");
        _log = new flashCache::Log(log_capacity, log_stats, readmit);
    }

    /* Initialize memory cache, size takes into account indexing overhead 
     * of index to log, and set nru tracking */
    uint64_t sets_memory_consumption = _sets->calcMemoryConsumption();
    uint64_t memory_size = (uint64_t) cfg.read<int>("cache.memorySizeMB") * 1024 * 1024;
    double perc_mem_log_overhead = cfg.read<float>("cache.memOverheadRatio", INDEX_LOG_RATIO);
    assert(log_capacity * perc_mem_log_overhead + sets_memory_consumption <= memory_size);
    uint64_t mem_cache_capacity = memory_size - (log_capacity * perc_mem_log_overhead);
    mem_cache_capacity -= sets_memory_consumption;
    std::cout << "Actual Memory Cache Size after indexing costs: " 
        << mem_cache_capacity << std::endl;
    auto& memory_cache_stats = statsCollector->createLocalCollector("memCache");
    _memCache = new memcache::LRU(mem_cache_capacity, memory_cache_stats);
        
    
    /* Initialize prelog admission policy */
    if (cfg.exists("preLogAdmission")) {
        std::string policyType = cfg.read<const char*>("preLogAdmission.policy");
        policyType.append(".preLogAdmission");
        const libconfig::Setting& admission_settings = cfg.read<libconfig::Setting&>("preLogAdmission");
        auto& admission_stats = statsCollector->createLocalCollector(policyType);
        _prelog_admission = admission::Policy::create(admission_settings, _sets, _log, admission_stats);
    }
    
    /* Initialize preset admission policy */
    if (cfg.exists("preSetAdmission")) {
        std::string policyType = cfg.read<const char*>("preSetAdmission.policy");
        policyType.append(".preSetAdmission");
        const libconfig::Setting& admission_settings = cfg.read<libconfig::Setting&>("preSetAdmission");
        auto& admission_stats = statsCollector->createLocalCollector(policyType);
        _preset_admission = admission::Policy::create(admission_settings, _sets, _log, admission_stats);
    }

    /* slow warmup */
    if (cfg.exists("cache.slowWarmup")) {
        warmed_up = true;
    }

    /* distribution num objects per set from log */
    if (cfg.exists("cache.recordSetDistribution")) {
        _record_dist = true;
        if (warmed_up) {
            _sets->enableDistTracking();
        }
    }
}

MemLogSetsCache::~MemLogSetsCache() {
    delete _sets;
    delete _log;
    delete _memCache;
    delete _prelog_admission;
    delete _preset_admission;
}
  

void MemLogSetsCache::insert(candidate_t id) {
    std::vector<candidate_t> ret = _memCache->insert(id);
    if (warmed_up && _prelog_admission) {
        auto admitted = _prelog_admission->admit(ret);
        ret.clear();
        for (auto set: admitted) {
            ret.insert(ret.end(), set.second.begin(), set.second.end());
        }
    } 
    ret = _log->insert(ret);
    if (ret.size()) {
        if (warmed_up && _preset_admission) {
            auto admitted = _preset_admission->admit(ret);
            for (auto set: admitted) {
                if (_record_dist) {
                    std::string num_name = "numItemsMoved" + std::to_string(set.second.size());
                    uint64_t size = std::accumulate(set.second.begin(), 
                            set.second.end(), 0, 
                            [](uint64_t sum, candidate_t next) {return (sum + next.obj_size);});
                    size = (size / SIZE_BUCKETING) * SIZE_BUCKETING;
                    std::string size_name = "sizeItemsMoved" + std::to_string(size);
                    globalStats[num_name]++;
                    globalStats[size_name]++;
                }
                ret = _sets->insert(set.first, set.second);
            }
        } else {
            ret = _sets->insert(ret);
        }
    }
    
    /* check warmed up condition every so often */
    if (!warmed_up && getAccessesAfterFlush() % CHECK_WARMUP_INTERVAL == 0) {
        checkWarmup();
    }
}

bool MemLogSetsCache::find(candidate_t id) {
    if (_memCache->find(id) || _log->find(id) || _sets->find(id)) {
        return true;
    }
    return false;
}

double MemLogSetsCache::calcFlashWriteAmp() {
    double set_write_amp = _sets->calcWriteAmp();
    if (warmed_up && _preset_admission) {
        set_write_amp *= _preset_admission->byteRatioAdmitted();
    }
    double flash_write_amp = set_write_amp + _log->calcWriteAmp(); 
    if (warmed_up && _prelog_admission) {
        /* include all bytes not written to structs */
        return flash_write_amp * _prelog_admission->byteRatioAdmitted();
    } else {
        return flash_write_amp;
    }
}

void MemLogSetsCache::checkWarmup() {
    if (_sets->ratioEvictedToCapacity() < 1) {
        return;
    }
    flushStats();
    std::cout << "Reached end of warmup, resetting stats\n\n";
    _sets->flushStats();
    _log->flushStats();
    _memCache->flushStats();
    dumpStats();
    warmed_up = true;
    _sets->enableDistTracking();
}

} // namespace cache
