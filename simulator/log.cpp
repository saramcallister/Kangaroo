#include "log.hpp"
#include "stats/stats.hpp"

namespace flashCache {

Log::Log(uint64_t log_capacity, stats::LocalStatsCollector& log_stats, uint64_t readmit):
        _log_stats(log_stats), 
        _total_capacity(log_capacity), 
        _total_size(0),
        _readmit(readmit) {
    _log_stats["logCapacity"] = _total_capacity;
}

void Log::_insert(candidate_t item) {
    _log_stats["bytes_written"] += item.obj_size;
    _log_stats["stores_requested"]++;
    _log_stats["stores_requested_bytes"] += item.obj_size;
    _total_size += item.obj_size;
    item.hit_count = 0;
    _items.insert(item);
    if (_readmit) {
        _per_item_hits[item] = 0;
    }
    assert(_total_size <= _total_capacity);
}

std::vector<candidate_t> Log::insert(std::vector<candidate_t> items) {
    std::vector<candidate_t> evicted;
    for (auto item: items) {
        if (item.obj_size + _total_size > _total_capacity) {
            std::copy(_items.begin(), _items.end(), std::back_inserter(evicted));
            _log_stats["numEvictions"] += evicted.size();
            _log_stats["sizeEvictions"] += _total_size;
            _log_stats["numLogFlushes"]++;
            _items.clear();
            _total_size = 0;
        }
        _insert(item);
    }
    _log_stats["current_size"] = _total_size;
    assert(_total_capacity >= _total_size);
    return evicted;
}

void Log::insertFromSets(candidate_t item) {
    if (item.obj_size + _total_size > _total_capacity) {
        _log_stats["bytes_rejected_from_sets"] += item.obj_size;
        _log_stats["num_rejected_from_sets"]++;
        return;
    }
    _log_stats["bytes_readmitted"] += item.obj_size;
    _log_stats["num_readmitted"]++;
    _log_stats["bytes_written"] += item.obj_size;
    _total_size += item.obj_size;
    _items.insert(item);
    assert(_total_size <= _total_capacity);
    _per_item_hits[item] = 0;
}

void Log::readmit(std::vector<candidate_t> items) {
    if (!_readmit) {
        return;
    }
    for (auto item: items) {
        if (_per_item_hits[item] > _readmit && _total_size + item.obj_size < _total_capacity) {
            _log_stats["bytes_written"] += item.obj_size;
            _log_stats["bytes_readmitted"] += item.obj_size;
            _log_stats["num_readmitted"]++;
            _total_size += item.obj_size;
            _items.insert(item);
            assert(_total_size <= _total_capacity);
            _per_item_hits[item] = 0;
        } else {
            _per_item_hits.erase(item);
        }
    }
    _log_stats["current_size"] = _total_size;
    assert(_total_capacity >= _total_size);
}

bool Log::find(candidate_t item) {
    auto it = _items.find(item);
    if (it == _items.end()) {
        _log_stats["misses"]++;
        return false;
    } else {
        _log_stats["hits"]++;
        it->hit_count++;
        if (_readmit) {
            _per_item_hits[item]++;
        }
        return true;
    }
}

double Log::ratioCapacityUsed() {
    return _total_size / _total_capacity;
}

double Log::calcWriteAmp() {
    double ret = _log_stats["bytes_written"]/ (double) _log_stats["stores_requested_bytes"];
    return ret;
}

void Log::flushStats() {
   _log_stats["bytes_written"] = 0;
   _log_stats["stores_requested"] = 0;
   _log_stats["stores_requested_bytes"] = 0;
   _log_stats["numEvictions"] = 0;
   _log_stats["sizeEvictions"] = 0;
   _log_stats["numLogFlushes"] = 0;
   _log_stats["misses"] = 0;
   _log_stats["hits"] = 0;
} 

}

