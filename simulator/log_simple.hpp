#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "candidate.hpp"
#include "log_abstract.hpp"
#include "stats/stats.hpp"

namespace flashCache {

// log, does lazy eviction in a round robin fashion as much as possible
class LogSimple : public virtual LogAbstract {

    public:
        LogSimple(uint64_t log_capacity, uint64_t block_size,
                stats::LocalStatsCollector& log_stats, uint64_t readmit);

        /* ----------- Basic functionality --------------- */

        /* insert multiple items (allows amortization of flash write), 
         * no guarantee for placement in multihash */
        std::vector<candidate_t> insert(std::vector<candidate_t> items);
        void insertFromSets(candidate_t item);
        
        /* returns true if the item is in sets layer */
        bool find(candidate_t item);
        
        /* ----------- Useful for Admission Policies --------------- */
        
        /* for readmission policies
         * shouldn't evict and only updates bytes written stats,
         * will handle "potential" evictions by remarking them as active
         * in the log even if they don't meet readmission threshold
         */
        void readmit(std::vector<candidate_t> items);

        /* returns ratio of total capacity used */
        double ratioCapacityUsed();
        
        /* ----------- Other Bookeeping --------------- */

        double calcWriteAmp();
        void flushStats(); // does not print update before clearing
        double ratioEvictedToCapacity();

    private:
        /* TODO: repetitive metadata structure, want to change in non-sim */
        stats::LocalStatsCollector& _log_stats;
        std::unordered_set<candidate_t> _items_fast;
        std::list<candidate_t> _items_ordered;
        uint64_t _total_capacity;
        uint64_t _total_size;

        std::vector<candidate_t> _evict(candidate_t item);
        void _insert(candidate_t item);
};


} // namespace flashCache
