#pragma once

#include <vector>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include "candidate.hpp"
#include "log_abstract.hpp"
#include "sets_abstract.hpp"
#include "stats/stats.hpp"

namespace flashCache {

// log, does lazy eviction in a round robin fashion as much as possible
class RotatingLog : public virtual LogAbstract {

    public:
        RotatingLog(uint64_t log_capacity, uint64_t block_size, SetsAbstract* sets,
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
        /* adds set matches (assumes 1 hash) to evicted and marks them
         * as duplicates in _item_to_block */
        std::vector<candidate_t> _addSetMatches(std::vector<candidate_t> evicted);

        SetsAbstract* _sets;
        stats::LocalStatsCollector& _log_stats;
        std::vector<Block> _blocks;
        /* whether object is unique to log (ie not in sets) */
        std::unordered_map<candidate_t, bool> _item_active;
        std::unordered_map<candidate_t, uint64_t> _per_item_hits;
        std::unordered_map<uint64_t, std::vector<candidate_t>> _set_to_items;
        uint64_t _total_capacity;
        uint64_t _total_size;
        uint64_t _active_block;
        uint64_t _num_blocks;
        bool _track_hits_per_item;
        uint64_t _readmit;
};


} // namespace flashCache
