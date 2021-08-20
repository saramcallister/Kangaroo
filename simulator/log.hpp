#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "candidate.hpp"
#include "log_abstract.hpp"
#include "stats/stats.hpp"

namespace flashCache {

// log, will evict everything together once it is full
class Log : public virtual LogAbstract {

    public:
        Log(uint64_t log_capacity, stats::LocalStatsCollector& log_stats, 
                uint64_t readmit);

        /* ----------- Basic functionality --------------- */

        /* insert multiple items (allows amortization of flash write), 
         * no guarantee for placement in multihash */
        std::vector<candidate_t> insert(std::vector<candidate_t> items);
        void insertFromSets(candidate_t item);
        
        /* returns true if the item is in sets layer */
        bool find(candidate_t item);
        
        /* ----------- Useful for Admission Policies --------------- */
        
        /* for readmission policies */
        void readmit(std::vector<candidate_t> items);

        /* returns ratio of total capacity used */
        double ratioCapacityUsed();
        
        /* ----------- Other Bookeeping --------------- */

        double calcWriteAmp();
        void flushStats();

    private:
        void _insert(candidate_t item);

        stats::LocalStatsCollector& _log_stats;
        std::unordered_set<candidate_t> _items;
        std::unordered_map<candidate_t, uint64_t> _per_item_hits;
        uint64_t _total_capacity;
        uint64_t _total_size;
        uint64_t _readmit;
};


} // namespace flashCache
