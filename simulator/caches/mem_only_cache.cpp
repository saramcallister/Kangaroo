#include "caches/mem_only_cache.hpp"
#include "config.hpp"
#include "lru.hpp"
#include "stats/stats.hpp"

namespace cache {

MemOnlyCache::MemOnlyCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings):
    Cache(sc, gs, settings) {
    misc::ConfigReader cfg(settings);
    
    /* Initialize memory cache */
    uint64_t memory_size_mb = (uint64_t) cfg.read<int>("cache.memorySizeMB");
    auto& memory_cache_stats = statsCollector->createLocalCollector("memCache");
    assert(!strcmp(cfg.read<const char*>("memoryCache.policy"),"LRU"));
    _memCache = new memcache::LRU(memory_size_mb * 1024 * 1024, memory_cache_stats);
}

MemOnlyCache::~MemOnlyCache() {
    delete _memCache;
}
  

void MemOnlyCache::insert(candidate_t id) {
    std::vector<candidate_t> ret = _memCache->insert(id);
}

bool MemOnlyCache::find(candidate_t id) {
    return _memCache->find(id);
}

double MemOnlyCache::calcFlashWriteAmp() {
   return 0; 
}

} // namespace cache
