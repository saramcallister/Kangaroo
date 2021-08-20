#pragma once
#include <libconfig.h++>

#include "admission/admission.hpp"
#include "caches/cache.hpp"
#include "sets_abstract.hpp"
#include "log_abstract.hpp"
#include "mem_cache.hpp"

namespace cache {

class MemLogSetsCache : public virtual Cache {
public:
    MemLogSetsCache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings);
    ~MemLogSetsCache();
    void insert(candidate_t id);
    bool find(candidate_t id);
    double calcFlashWriteAmp();
    double calcMissRate();

    void readmitToLogFromSets(candidate_t item) {
        _log->insertFromSets(item);
    }


private:
    void checkWarmup();

    flashCache::SetsAbstract* _sets = nullptr;
    flashCache::LogAbstract* _log = nullptr;
    memcache::MemCache* _memCache = nullptr;
    admission::Policy* _prelog_admission = nullptr;
    admission::Policy* _preset_admission = nullptr;
    bool warmed_up = false;
    bool _record_dist = false;
};

} // namespace cache
