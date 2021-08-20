#include "constants.hpp"
#include "log_only.hpp"
#include "stats/stats.hpp"

namespace flashCache {

LogOnly::LogOnly(uint64_t log_capacity, uint64_t block_size,
    stats::LocalStatsCollector& log_stats, uint64_t readmit):
        _log_stats(log_stats), 
        _total_capacity(log_capacity), 
        _total_size(0),
        _active_block(0),
        _readmit(readmit) {
    _log_stats["logCapacity"] = _total_capacity;
    _num_blocks = log_capacity / block_size;
    Block template_block = Block(block_size);
    _blocks.resize(_num_blocks, template_block);
    // allow last block to be smaller than the others
    if (log_capacity % block_size) {
        _num_blocks++;
        template_block._capacity = log_capacity % block_size;
        _blocks.push_back(template_block);
    }
    std::cout << "Log capacity: " << _total_capacity
        << "\n\tNum Blocks: " << _num_blocks
        << "\n\tBlock Capacity: " << block_size << std::endl;
}

void LogOnly::_insert(candidate_t item) {
    _log_stats["bytes_written"] += item.obj_size;
    _log_stats["stores_requested"]++;
    _log_stats["stores_requested_bytes"] += item.obj_size;
    assert(_item_active.find(item) == _item_active.end());
    _total_size += item.obj_size;
    item.hit_count = 0;
    _blocks[_active_block].insert(item);
    _item_active[item] = true;
    assert(_blocks[_active_block]._size <= _blocks[_active_block]._capacity);
    _num_inserts++;
    _size_inserts += item.obj_size;
    assert(_size_inserts == _blocks[_active_block]._size);
}

std::vector<candidate_t> LogOnly::_incrementBlockAndFlush() {
    std::vector<candidate_t> evicted;
    _active_block = (_active_block + 1) % _num_blocks;
    Block& current_block = _blocks[_active_block];

    if (current_block._size) {
        evicted.reserve(current_block._items.size());
        for (auto item: current_block._items) {
            if (_item_active[item]) {
                // only move if not already in sets
                evicted.push_back(item);
            }
            // should always remove an item, otherwise code bug
            _item_active.erase(item);
        }
        _log_stats["numEvictions"] += current_block._items.size();
        _log_stats["sizeEvictions"] += current_block._size;
        _log_stats["numLogFlushes"]++;
        _total_size -= current_block._size;
        current_block._items.clear();
        current_block._size = 0;
    }
    if (_active_block == 77) {
    }
    _num_inserts = 0;
    _size_inserts = 0;
    return evicted;
}

std::vector<candidate_t> LogOnly::insert(std::vector<candidate_t> items) {
    std::vector<candidate_t> evicted;
    for (auto item: items) {
        Block& current_block = _blocks[_active_block];
        if (item.obj_size + current_block._size > current_block._capacity) {
            /* move active block pointer */
            std::vector<candidate_t> local_evict = _incrementBlockAndFlush();
            evicted.insert(evicted.end(), local_evict.begin(), local_evict.end());
        }
        _insert(item);
    }
    assert(_total_capacity >= _total_size);
    _log_stats["current_size"] = _total_size;
    return evicted;
}

void LogOnly::readmit(std::vector<candidate_t> items) {
    return;
}

void LogOnly::insertFromSets(candidate_t item) {
    return;
}

bool LogOnly::find(candidate_t item) {
    auto it = _item_active.find(item);
    if (it == _item_active.end()) {
        _log_stats["misses"]++;
        return false;
    } else {
        _log_stats["hits"]++;
        return true;
    }
}

double LogOnly::ratioCapacityUsed() {
    return _total_size / _total_capacity;
}

double LogOnly::calcWriteAmp() {
    double ret = _log_stats["bytes_written"]/ (double) _log_stats["stores_requested_bytes"];
    return ret;
}

void LogOnly::flushStats() {
    _log_stats["bytes_written"] = 0;
    _log_stats["stores_requested"] = 0;
    _log_stats["stores_requested_bytes"] = 0;
    _log_stats["numEvictions"] = 0;
    _log_stats["sizeEvictions"] = 0;
    _log_stats["numLogFlushes"] = 0;
    _log_stats["misses"] = 0;
    _log_stats["hits"] = 0;
    _log_stats["num_early_evict"] = 0;
    _log_stats["size_early_evict"] = 0;
    _log_stats["bytes_rejected_from_sets"] = 0;
    _log_stats["num_rejected_from_sets"] = 0;
} 

double LogOnly::ratioEvictedToCapacity() {
    return _log_stats["sizeEvictions"] / _total_capacity;
}

} // namespace flashCache

