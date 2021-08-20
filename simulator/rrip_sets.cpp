#include <list>
#include <math.h>

#include "caches/mem_log_sets_cache.hpp"
#include "constants.hpp"
#include "rrip_sets.hpp"
#include "stats/stats.hpp"

namespace flashCache {

RripSets::RripSets(uint64_t num_sets, uint64_t set_capacity, 
    stats::LocalStatsCollector& set_stats, cache::MemLogSetsCache* ref_cache, 
    int num_hash_functions, int bits, bool promotion_rrip, bool mixed_rrip):
        _set_stats(set_stats), 
        _set_capacity(set_capacity), 
        _num_sets(num_sets), 
        _total_size(0),
        _total_capacity(set_capacity * num_sets),
        _cache(ref_cache),
        _num_hash_functions(num_hash_functions),
        _bits(bits),
        _max_rrpv(exp2(bits) - RRIP_DISTANT_DIFF),
        _mixed(mixed_rrip),
        _promotion_only(promotion_rrip) {
    _bins.resize(_num_sets);
    _set_stats["numSets"] = _num_sets;
    _set_stats["setCapacity"] = _set_capacity;
    _set_stats["numHashFunctions"] = _num_hash_functions;
    _set_stats["rripBits"] = bits;
}

void RripSets::_incrementRrpvValues(uint64_t bin_num) {
    Bin &bin = _bins[bin_num];
    if (bin.rrpv_to_items.empty()) {
        return;
    } 

    /* increment highest key to max rrpv value */
    int current_max = (bin.rrpv_to_items.begin())->first;
    int diff = _max_rrpv - current_max;
    if (diff <= 0) {
        return;
    }

    auto it = bin.rrpv_to_items.begin();
    while (it != bin.rrpv_to_items.end()) {
        bin.rrpv_to_items[it->first + diff] = it->second;
        it = bin.rrpv_to_items.erase(it);
    } 
}

int64_t RripSets::_calcAllowableSize(Bin &bin, int insertion_point) {
    int64_t allowable_size = 0;
    for (auto it = bin.rrpv_to_items.begin(); it != bin.rrpv_to_items.end(); it++) {
        if (it->first < insertion_point) {
            break;
        }
        allowable_size += std::accumulate(it->second.begin(), it->second.end(), 0, 
                [](int64_t a, candidate_t b){return a + b.obj_size;});
    }
    return allowable_size;
}

std::vector<candidate_t> RripSets::_insert(candidate_t item, uint64_t bin_num) {
    assert((uint64_t) item.obj_size <= _set_capacity);
    std::unordered_set<uint64_t> potential_bins = findSetNums(item);
    assert(potential_bins.find(bin_num) != potential_bins.end());
    
    std::vector<candidate_t> evicted;
    assert(!(item.obj_size == 0 && item.id == 0));
    Bin &bin = _bins[bin_num];

    int insert_val = _max_rrpv - RRIP_LONG_DIFF - item.hit_count;
    if (insert_val < 0) {
        insert_val = 0;
    } else if (_promotion_only && item.hit_count) {
        insert_val = 0;
    }
    
    if (_calcAllowableSize(bin, insert_val) < item.obj_size 
            && (uint64_t) item.obj_size + bin.bin_size > _set_capacity) {
        _set_stats["numEvictions"]++;
        _set_stats["sizeEvictions"] += item.obj_size;
        _set_stats["numEvictionsImmediate"]++;
        _set_stats["sizeEvictionsImmediate"] += item.obj_size;
        if (item.hit_count) {
            _cache->readmitToLogFromSets(item);
        } else {
            evicted.push_back(item);
        }
        return evicted;
    }

    while ((uint64_t) item.obj_size + bin.bin_size > _set_capacity) {
        assert(!bin.rrpv_to_items.begin()->second.empty());
        candidate_t old = bin.rrpv_to_items.begin()->second.front();
        _set_stats["numEvictions"]++;
        _set_stats["sizeEvictions"] += old.obj_size;
        bin.bin_size -= old.obj_size;
        _total_size -= old.obj_size;
        evicted.push_back(old);
        bin.rrpv_to_items.begin()->second.pop_front();
        if (bin.rrpv_to_items.begin()->second.empty()) {
            bin.rrpv_to_items.erase(bin.rrpv_to_items.begin());
        }
    }

    bin.rrpv_to_items[insert_val].push_back(item);
    bin.bin_size += item.obj_size;
    _total_size += item.obj_size;
    _set_stats["current_size"] = _total_size;
    return evicted;
}

std::vector<candidate_t> RripSets::insert(std::vector<candidate_t> items) {
    std::vector<bool> sets_touched(_num_sets, false);
    std::vector<candidate_t> evicted;
    for (auto item: items) {
        uint64_t bin_num = *findSetNums(item).begin();
        assert(bin_num < _num_sets);
        if (!sets_touched[bin_num]) {
            _incrementRrpvValues(bin_num);
        }
        sets_touched[bin_num] = true;
        std::vector<candidate_t> local_evict = _insert(item, bin_num);
        evicted.insert(evicted.end(), local_evict.begin(), local_evict.end());
        _updateStatsRequestedStore(item);
    }
    uint64_t num_sets_touched = std::count(sets_touched.begin(), sets_touched.end(), true);
    _updateStatsActualStore(num_sets_touched);
    assert(_total_capacity >= _total_size);
    return evicted;
}

std::vector<candidate_t> RripSets::insert(uint64_t set_num, std::vector<candidate_t> items) {
    std::vector<candidate_t> evicted;
    assert(set_num < _num_sets);
    _incrementRrpvValues(set_num);
    for (auto item: items) {
        std::vector<candidate_t> local_evict = _insert(item, set_num);
        evicted.insert(evicted.end(), local_evict.begin(), local_evict.end());
        _updateStatsRequestedStore(item);
    }
    if (items.size() > 0) {
        _updateStatsActualStore(1);
    }
    assert(_total_capacity >= _total_size);
    return evicted;
}

void RripSets::_updateStatsActualStore(uint64_t num_sets_updated) {
    _set_stats["bytes_written"] += (num_sets_updated * _set_capacity);
}

void RripSets::_updateStatsRequestedStore(candidate_t item) {
    _set_stats["stores_requested"]++;
    _set_stats["stores_requested_bytes"] += item.obj_size;
}

bool RripSets::find(candidate_t item) {
    std::unordered_set<uint64_t> possible_bins = findSetNums(item);
    for (uint64_t bin_num: possible_bins) {
        auto it = _bins[bin_num].rrpv_to_items.begin();
        while (it != _bins[bin_num].rrpv_to_items.end()) {
            for (auto vec_it = it->second.begin(); vec_it != it->second.end(); vec_it++) {
                if (*vec_it == item) {
                    if (it->first != 0) {
                        if (_mixed || _promotion_only) {
                            _bins[bin_num].rrpv_to_items[0].push_back(*vec_it);
                        } else {
                            _bins[bin_num].rrpv_to_items[it->first - 1].push_back(*vec_it);
                        }
                        it->second.erase(vec_it);
                        if (it->second.empty()) {
                            _bins[bin_num].rrpv_to_items.erase(it);
                        }
                    }
                    _set_stats["hits"]++;
                    if (_hit_dist) {
                        std::string name = "setHits" + std::to_string(bin_num);
                        _set_stats[name]++;
                    }
                    return true;
                }
            }
            it++;
        }
        if (_hit_dist) {
            std::string name = "setMisses" + std::to_string(bin_num);
            _set_stats[name]++;
        }
    }
    _set_stats["misses"]++;
    return false;
}

std::unordered_set<uint64_t> RripSets::findSetNums(candidate_t item) {
    std::unordered_set<uint64_t> possibilities;
    uint64_t current_value = (uint64_t) item.id;
    for (int i = 0; i <= _num_hash_functions; i++) {
        possibilities.insert(current_value % _num_sets);
        current_value = std::hash<std::string>{}(std::to_string(std::hash<uint64_t>{}(current_value)));
    }
    return possibilities;
}

double RripSets::ratioCapacityUsed() {
    return _total_size / (double) _total_capacity;
}

double RripSets::calcWriteAmp() {
    return _set_stats["bytes_written"]/ (double) _set_stats["stores_requested_bytes"];
}

double RripSets::ratioEvictedToCapacity() {
    return _set_stats["sizeEvictions"]/ (double) _total_capacity;
}

void RripSets::flushStats() {
    _set_stats["misses"] = 0;
    _set_stats["hits"] = 0;
    _set_stats["bytes_written"] = 0;
    _set_stats["stores_requested"] = 0;
    _set_stats["stores_requested_bytes"] = 0;
    _set_stats["sizeEvictions"] = 0;
    _set_stats["numEvictions"] = 0;
    _set_stats["hitsSharedWithLog"] = 0;
    _set_stats["trackHitsFailed"] = 0;
    _set_stats["numHitItemsEvicted"] = 0;
}

uint64_t RripSets::calcMemoryConsumption() {
    /* TODO: add bloom filters */
    /* this is a strong estimate based on avg obj_size */
    double bits_per_set = _bits * (_set_capacity / AVG_OBJ_SIZE_BYTES);
    uint64_t sets_memory_consumption_bits = (uint64_t) bits_per_set * _num_sets;
    return sets_memory_consumption_bits / 8;
}

bool RripSets::trackHit(candidate_t item) {
    std::unordered_set<uint64_t> possible_bins = findSetNums(item);
    for (uint64_t bin_num: possible_bins) {
        auto it = _bins[bin_num].rrpv_to_items.begin();
        while (it != _bins[bin_num].rrpv_to_items.end()) {
            for (auto vec_it = it->second.begin(); vec_it != it->second.end(); vec_it++) {
                if (*vec_it == item) {
                    if (it->first != 0) {
                        if (_mixed || _promotion_only) {
                            _bins[bin_num].rrpv_to_items[0].push_back(*vec_it);
                        } else {
                            _bins[bin_num].rrpv_to_items[it->first - 1].push_back(*vec_it);
                        }
                        it->second.erase(vec_it);
                        if (it->second.empty()) {
                            _bins[bin_num].rrpv_to_items.erase(it);
                        }
                    }
                    _set_stats["hitsSharedWithLog"]++;
                    return true;
                }
            }
            it++;
        }
    }
    _set_stats["trackHitsFailed"]++;
    return false;
}

void RripSets::enableDistTracking() {
    _dist_tracking = true;
}

void RripSets::enableHitDistributionOverSets() {
    _hit_dist = true;
}

} // namespace flashCache

