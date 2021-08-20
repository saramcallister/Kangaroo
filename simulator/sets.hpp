#pragma once

#include <deque>
#include <vector>
#include <unordered_set>
#include "candidate.hpp"
#include "log_abstract.hpp"
#include "sets_abstract.hpp"
#include "stats/stats.hpp"

namespace cache {
    class MemLogSetsCache;
}

namespace flashCache {

class Sets : public virtual SetsAbstract {

    public:
        Sets(uint64_t num_sets, uint64_t set_capacity, 
                stats::LocalStatsCollector& set_stats, cache::MemLogSetsCache* ref_cache = nullptr, 
                int num_hash_functions = 1, bool nru=false);
        ~Sets() {}

        /* ----------- Basic functionality --------------- */

        /* insert multiple items (allows amortization of flash write), 
         * no guarantee for placement in multihash */
        std::vector<candidate_t> insert(std::vector<candidate_t> items);
        
        /* 
         * insert multiple items into the same set, useful for multihash
         * will fail assert if any items cannot be matched to provided set_num
         */
        std::vector<candidate_t> insert(uint64_t set_num, std::vector<candidate_t> items);
        
        /* returns true if the item is in sets layer */
        bool find(candidate_t item);
        
        /* ----------- Useful for Admission Policies --------------- */

        /* returns the set numbers that item could be mapped to */
        std::unordered_set<uint64_t> findSetNums(candidate_t item);

        /* returns ratio of total capacity used */
        double ratioCapacityUsed();

        /* ----------- Other Bookeeping --------------- */

        double calcWriteAmp();
        double ratioEvictedToCapacity();
        void flushStats(); // does not print update before clearing
        uint64_t calcMemoryConsumption();
        bool trackHit(candidate_t item); // hit on object in sets elsewhere in cache, for nru
        void enableDistTracking();
        void enableHitDistributionOverSets();

    private:
        struct Bin {
            std::deque<candidate_t> items;
            int64_t bin_size;
            uint64_t no_hit_insert_loc; // where to put non-hit log items
        };
        stats::LocalStatsCollector& _set_stats;
        std::vector<Bin> _bins;
        std::vector<std::vector<bool>> _hits_in_sets;
        uint64_t _set_capacity;
        uint64_t _num_sets;
        uint64_t _total_size;
        uint64_t _total_capacity;
        cache::MemLogSetsCache* _cache;
        int _num_hash_functions;
        bool _nru;
        bool _dist_tracking = false;
        bool _hit_dist = false;

        /* 
         * internal insert, ensures that a bucket is not overfull
         * does not update stats, errors if bin_num and item do not match
         */
        std::vector<candidate_t> _insert(candidate_t item, uint64_t bin_num);

        /* reorder objects based on nru status, clears nru bits for set */
        uint64_t _reorder_set_nru(uint64_t bin_num);

        /* update stats related to flash writes */
        void _updateStatsActualStore(uint64_t num_sets_updated);
        void _updateStatsRequestedStore(candidate_t item);
};


} // namespace flashCache
