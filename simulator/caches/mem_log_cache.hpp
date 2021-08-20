#pragma once
#include <libconfig.h++>

#include "admission/admission.hpp"
#include "caches/cache.hpp"
#include "sets_abstract.hpp"
#include "log_only.hpp"
#include "lru.hpp"

namespace cache {

class MemLogCache : public virtual Cache {
public:
    MemLogCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings);
    ~MemLogCache();
    void insert(candidate_t id);
    bool find(candidate_t id);
    double calcFlashWriteAmp();
    double calcMissRate();

private:
    void checkWarmup();

    flashCache::LogOnly* _log = nullptr;
    memcache::LRU* _memCache = nullptr;
    admission::Policy* _prelog_admission = nullptr;
    bool warmed_up = false;
    bool _record_dist = false;
};

} // namespace cache
