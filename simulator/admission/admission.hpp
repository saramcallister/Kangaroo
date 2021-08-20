#pragma once

#include <libconfig.h++>
#include <vector>

#include "log_abstract.hpp"
#include "sets_abstract.hpp"
#include "stats/stats.hpp"

namespace admission {

class Policy {
public:
    Policy(stats::LocalStatsCollector& admission_stats, flashCache::SetsAbstract* sets = nullptr, 
            flashCache::LogAbstract* log = nullptr);
    virtual ~Policy() {}
    virtual std::unordered_map<uint64_t, std::vector<candidate_t>> admit(std::vector<candidate_t> items) = 0;
    virtual std::vector<candidate_t> admit_simple(std::vector<candidate_t> items) = 0;

    /* the settings here is assumed to just be for the specific admission policy 
     * since multiple admission policies are possible and the cache should decide how 
     * to concatenate them */
    static Policy* create(const libconfig::Setting& settings, flashCache::SetsAbstract* sets, 
            flashCache::LogAbstract* log, stats::LocalStatsCollector& admission_stats);

    /* For write amp calculations */
    double byteRatioAdmitted();

protected:
    stats::LocalStatsCollector& _admission_stats;

    /* Very simple group, just takes first possible set for object, 
     * works perfectly for one hash, TODO: implemeent something better for multihash */
    std::unordered_map<uint64_t, std::vector<candidate_t>> groupBasic(std::vector<candidate_t> items);

    /* Useful stats tracking for derived classes */
    void trackPossibleAdmits(std::vector<candidate_t> items);
    void trackAdmitted(std::vector<candidate_t> items);

    /* not all policies (eg random) need these so they can be nullptrs all this level */
    flashCache::SetsAbstract* _sets;
    flashCache::LogAbstract* _log;

    /* for readmission, call only once */
    void performReadmission(std::vector<candidate_t> evicted);
};


} // namespace admission
