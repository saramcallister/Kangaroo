#pragma once
#include <libconfig.h++>
#include "caches/cache.hpp"
#include "lru.hpp"

namespace cache {

class MemOnlyCache : public virtual Cache {
public:
    MemOnlyCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings);
    ~MemOnlyCache();
    void insert(candidate_t id);
    bool find(candidate_t id);
    double calcFlashWriteAmp();
    double calcMissRate();

private:
    memcache::LRU* _memCache = nullptr;
};

} // namespace cache
