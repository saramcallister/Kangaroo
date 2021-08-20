#pragma once
#include <libconfig.h++>
#include "admission/admission.hpp"
#include "caches/cache.hpp"
#include "sets_abstract.hpp"
#include "lru.hpp"

namespace cache {

class SetOnlyCache : public virtual Cache {
public:
    SetOnlyCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings);
    ~SetOnlyCache();
    void insert(candidate_t id);
    bool find(candidate_t id);
    double calcFlashWriteAmp();
    double calcMissRate();

private:
    void checkWarmup();

    flashCache::SetsAbstract* _sets = nullptr;
    memcache::LRU* _memCache = nullptr;
    admission::Policy* _preset_admission = nullptr;
    bool warmed_up = false;
};

} // namespace cache
