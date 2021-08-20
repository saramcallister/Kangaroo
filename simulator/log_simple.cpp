#include "constants.hpp"
#include "log_simple.hpp"
#include "stats/stats.hpp"

namespace flashCache {

LogSimple::LogSimple(uint64_t log_capacity, uint64_t block_size,
    stats::LocalStatsCollector& log_stats, uint64_t readmit):
        _log_stats(log_stats), 
        _total_capacity(log_capacity), 
        _total_size(0) {
    _log_stats["logCapacity"] = _total_capacity;
}

void LogSimple::_insert(candidate_t item) {
    _log_stats["bytes_written"] += item.obj_size;
    _log_stats["stores_requested"]++;
    _log_stats["stores_requested_bytes"] += item.obj_size;
    _total_size += item.obj_size;
    assert(_total_size <= _total_capacity);
    assert(_items_fast.find(item) == _items_fast.end());
    _items_fast.insert(item);
    _items_ordered.push_back(item);
}

std::vector<candidate_t> LogSimple::_evict(candidate_t item) {
    std::vector<candidate_t> ret;
    while (item.obj_size + _total_size > _total_capacity) {
        candidate_t evicted_item = _items_ordered.front();
        _items_fast.erase(evicted_item);
        _total_size -= evicted_item.obj_size;
        _log_stats["sizeEvictions"] += evicted_item.obj_size;
        ret.push_back(evicted_item);
        _items_ordered.pop_front();
    }
    _log_stats["numEvictions"] += ret.size();
    assert(_total_size + item.obj_size <= _total_capacity);
    return ret;
}

std::vector<candidate_t> LogSimple::insert(std::vector<candidate_t> items) {
    std::vector<candidate_t> evicted;
    for (auto item: items) {
        if (item.obj_size + _total_size >_total_capacity) {
            /* move active block pointer */
            std::vector<candidate_t> local_evict = _evict(item);
            evicted.insert(evicted.end(), local_evict.begin(), local_evict.end());
        }
        _insert(item);
    }
    assert(_total_capacity >= _total_size);
    _log_stats["current_size"] = _total_size;
    return evicted;
}

void LogSimple::readmit(std::vector<candidate_t> items) {
    return;
}

void LogSimple::insertFromSets(candidate_t item) {
    return;
}

bool LogSimple::find(candidate_t item) {
    auto it = _items_fast.find(item);
    if (it == _items_fast.end()) {
        _log_stats["misses"]++;
        return false;
    } else {
        _log_stats["hits"]++;
        return true;
    }
}

double LogSimple::ratioCapacityUsed() {
    return _total_size / _total_capacity;
}

double LogSimple::calcWriteAmp() {
    double ret = _log_stats["bytes_written"]/ (double) _log_stats["stores_requested_bytes"];
    return ret;
}

void LogSimple::flushStats() {
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

double LogSimple::ratioEvictedToCapacity() {
    return _log_stats["sizeEvictions"] / _total_capacity;
}

} // namespace flashCache

