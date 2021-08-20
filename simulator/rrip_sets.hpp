#pragma once

#include <deque>
#include <functional>
#include <map>
#include <vector>

#include "candidate.hpp"
#include "log_abstract.hpp"
#include "sets_abstract.hpp"
#include "stats/stats.hpp"

namespace cache {
    class MemLogSetsCache;
}

namespace flashCache {

class RripSets : public virtual SetsAbstract {

    public:
        RripSets(uint64_t num_sets, uint64_t set_capacity, 
                stats::LocalStatsCollector& set_stats, cache::MemLogSetsCache* ref_cache = nullptr, 
                int num_hash_functions = 1, int bits = 2, bool promotion_rrip = false,
                bool mixed_rrip = false);
        ~RripSets() {}

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
            // rrpv = re-reference pointer value
            std::map<int, std::deque<candidate_t>, std::greater<int>> rrpv_to_items;
            int64_t bin_size;
        };
        stats::LocalStatsCollector& _set_stats;
        std::vector<Bin> _bins;
        uint64_t _set_capacity;
        uint64_t _num_sets;
        uint64_t _total_size;
        uint64_t _total_capacity;
        cache::MemLogSetsCache* _cache;
        int _num_hash_functions;
        int _bits;
        int _max_rrpv;
        bool _dist_tracking = false;
        bool _hit_dist = false;

        /* 
         * control promotion rrip where items reset to 0 on hit instead
         * of decrementing value
         */
        bool _mixed = false; // use promotion rrip in sets, but val rrip in merge
        bool _promotion_only = false; // promotion rrip for everythion

        /* 
         * internal insert, ensures that a bucket is not overfull
         * does not update stats, errors if bin_num and item do not match
         */
        std::vector<candidate_t> _insert(candidate_t item, uint64_t bin_num);

        int64_t _calcAllowableSize(Bin &bin, int insertion_point);
        void _incrementRrpvValues(uint64_t bin_num);

        /* update stats related to flash writes */
        void _updateStatsActualStore(uint64_t num_sets_updated);
        void _updateStatsRequestedStore(candidate_t item);
};


} // namespace flashCache
