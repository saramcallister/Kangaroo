#pragma once
#include <libconfig.h++>
#include "parsers/parser.hpp"
#include "stats/stats.hpp"
#include "candidate.hpp"

namespace cache {

// Note: because candidates do not have the same size, in general:
// 
//    accesses != hits + evictions + fills     (not equal!)
// 
// A single access can result in multiple evictions to make space for
// the new object. That is, evictions != misses.
// 
// Fills is the number of misses that don't require an eviction, that
// is there is sufficient available space to fit the object. Evictions
// is the number of evictions, _not_ the number of misses that lead to
// an eviction.
class Cache {
public:
    Cache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings);
    virtual ~Cache();

    /* create propor subclass of cache given settings */
    static Cache* create(const libconfig::Setting &settings);

    /* access method, calls insert and find */
    void access(const parser::Request& req);

    virtual void insert(candidate_t id) = 0;
    virtual bool find(candidate_t id) = 0;
    virtual double calcFlashWriteAmp() = 0;
    
    double calcMissRate();
    
    /* dumpStats to predefined stats file */
    void dumpStats();
    uint64_t getTotalAccesses();
    uint64_t getAccessesAfterFlush();


protected:
    /* useful functions that are common to caches */
    void trackHistory(candidate_t id);
    void trackAccesses();
    void flushStats();
    stats::StatsCollector* statsCollector = nullptr;
    stats::LocalStatsCollector& globalStats;

private:
    CandidateMap<bool> _historyAccess;
    uint64_t _stats_interval;

}; // class Cache

}
