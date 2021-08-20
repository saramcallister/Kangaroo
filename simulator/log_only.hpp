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
class LogOnly : public virtual LogAbstract {

    public:
        LogOnly(uint64_t log_capacity, uint64_t block_size,
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
        struct Block {
            std::unordered_set<candidate_t> _items;
            uint64_t _capacity;
            uint64_t _size;

            Block(uint64_t capacity): _capacity(capacity), _size(0) {}

            void insert(candidate_t item) {
                _items.insert(item);
                _size += item.obj_size;
                assert(_size <= _capacity);
            }
        };

        void _insert(candidate_t item);
        /* will only return forced evictions */
        std::vector<candidate_t> _incrementBlockAndFlush();

        stats::LocalStatsCollector& _log_stats;
        std::vector<Block> _blocks;
        std::unordered_map<candidate_t, bool> _item_active;
        uint64_t _total_capacity;
        uint64_t _total_size;
        uint64_t _active_block;
        uint64_t _num_blocks;
        bool _track_hits_per_item;
        uint64_t _readmit;
        uint64_t _num_inserts = 0;
        uint64_t _size_inserts = 0;
};


} // namespace flashCache
