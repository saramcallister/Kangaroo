#include "caches/set_only_cache.hpp"
#include "config.hpp"
#include "constants.hpp"
#include "lru.hpp"
#include "rrip_sets.hpp"
#include "sets.hpp"
#include "stats/stats.hpp"

namespace cache {

SetOnlyCache::SetOnlyCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings):
    Cache(sc, gs, settings) {
    misc::ConfigReader cfg(settings);
    
    /* Initialize flash cache */
    uint64_t set_capacity = (uint64_t) cfg.read<int>("sets.setCapacity");
    uint64_t flash_size_mb = (uint64_t) cfg.read<int>("cache.flashSizeMB");
    uint64_t num_sets = (flash_size_mb * 1024 * 1024) / set_capacity;
    int num_hash_functions = cfg.read<int>("sets.numHashFunctions", 1);
    auto& set_stats = statsCollector->createLocalCollector("sets");
    if (cfg.exists("sets.rripBits")) {
        int rrip_bits = cfg.read<int>("sets.rripBits");
        bool promotion = (bool) cfg.read<int>("sets.promotionOnly", 0);
        bool mixed = (bool) cfg.read<int>("sets.mixedRRIP", 0);
        _sets = new flashCache::RripSets(num_sets, set_capacity, set_stats, nullptr, 
                num_hash_functions, rrip_bits, promotion, mixed);
    } else {
        bool sets_track_hits = cfg.exists("sets.trackHitsPerItem");
        _sets = new flashCache::Sets(num_sets, set_capacity, set_stats, nullptr, num_hash_functions, sets_track_hits);
    }

    /* Initialize memory cache */
    uint64_t sets_memory_consumption = _sets->calcMemoryConsumption();
    uint64_t memory_size = (uint64_t) cfg.read<int>("cache.memorySizeMB") * 1024 * 1024;
    assert(sets_memory_consumption <= memory_size);
    memory_size -= sets_memory_consumption;
    auto& memory_cache_stats = statsCollector->createLocalCollector("memCache");
    assert(!strcmp(cfg.read<const char*>("memoryCache.policy"),"LRU"));
    _memCache = new memcache::LRU(memory_size, memory_cache_stats);
    
    /* Initialize preset admission policy */
    if (cfg.exists("preSetAdmission")) {
        std::string policyType = cfg.read<const char*>("preSetAdmission.policy");
        policyType.append(".preSetAdmission");
        const libconfig::Setting& admission_settings = cfg.read<libconfig::Setting&>("preSetAdmission");
        auto& admission_stats = statsCollector->createLocalCollector(policyType);
        _preset_admission = admission::Policy::create(admission_settings, _sets, nullptr, admission_stats);
    }
    
    /* slow warmup */
    if (cfg.exists("cache.slowWarmup")) {
        warmed_up = true;
    }
}

SetOnlyCache::~SetOnlyCache() {
    delete _sets;
    delete _memCache;
    delete _preset_admission;
}
  

void SetOnlyCache::insert(candidate_t id) {
    std::vector<candidate_t> ret = _memCache->insert(id);
    if (warmed_up && _preset_admission) {
        auto admitted = _preset_admission->admit(ret);
        for (auto set: admitted) {
            _sets->insert(set.first, set.second);
        }
    } else if (ret.size() != 0) {
        _sets->insert(ret);
    }
    
    if (!warmed_up && getAccessesAfterFlush() % CHECK_WARMUP_INTERVAL == 0) {
        checkWarmup();
    }
}

bool SetOnlyCache::find(candidate_t id) {
    return _memCache->find(id) || _sets->find(id);
}

double SetOnlyCache::calcFlashWriteAmp() {
    double set_write_amp = _sets->calcWriteAmp();
    if (warmed_up && _preset_admission) {
        set_write_amp *= _preset_admission->byteRatioAdmitted();
    }
   return set_write_amp; 
}

void SetOnlyCache::checkWarmup() {
    if (_sets->ratioEvictedToCapacity() < 1) {
        return;
    }
    flushStats();
    std::cout << "Reached end of warmup, resetting stats\n\n";
    _sets->flushStats();
    _memCache->flushStats();
    dumpStats();
    warmed_up = true;
}

} // namespace cache
