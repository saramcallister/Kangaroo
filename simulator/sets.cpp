#include <list>

#include "caches/mem_log_sets_cache.hpp"
#include "constants.hpp"
#include "sets.hpp"
#include "stats/stats.hpp"

namespace flashCache {

Sets::Sets(uint64_t num_sets, uint64_t set_capacity, 
    stats::LocalStatsCollector& set_stats, cache::MemLogSetsCache* ref_cache, 
    int num_hash_functions, bool nru):
        _set_stats(set_stats), 
        _set_capacity(set_capacity), 
        _num_sets(num_sets), 
        _total_size(0),
        _total_capacity(set_capacity * num_sets),
        _cache(ref_cache),
        _num_hash_functions(num_hash_functions),
        _nru(nru) {
    _bins.resize(_num_sets);
    _set_stats["numSets"] = _num_sets;
    _set_stats["setCapacity"] = _set_capacity;
    _set_stats["numHashFunctions"] = _num_hash_functions;
    _set_stats["nru"] = _nru;
    if (_nru) {
        _hits_in_sets.resize(_num_sets);
        for (auto& set_hits: _hits_in_sets) {
            set_hits.resize(HIT_BIT_VECTOR_SIZE, false);
        }
    }
}

std::vector<candidate_t> Sets::_insert(candidate_t item, uint64_t bin_num) {
    assert((uint64_t) item.obj_size <= _set_capacity);
    std::unordered_set<uint64_t> potential_bins = findSetNums(item);
    assert(potential_bins.find(bin_num) != potential_bins.end());
    
    std::vector<candidate_t> evicted;
    assert(!(item.obj_size == 0 && item.id == 0));
    Sets::Bin &bin = _bins[bin_num];
    assert(std::find(bin.items.begin(), bin.items.end(), item) == bin.items.end());
    while ((uint64_t) item.obj_size + bin.bin_size > _set_capacity) {
        if (bin.no_hit_insert_loc == 0 && !item.hit_count) {
            _set_stats["numEvictions"]++;
            _set_stats["sizeEvictions"] += item.obj_size;
            _set_stats["numEvictionsImmediate"]++;
            _set_stats["sizeEvictionsImmediate"] += item.obj_size;
            evicted.push_back(item);
            return evicted;
        }
        candidate_t old = bin.items.front();
        _set_stats["numEvictions"]++;
        _set_stats["sizeEvictions"] += old.obj_size;
        bin.bin_size -= old.obj_size;
        _total_size -= old.obj_size;
        if (bin.no_hit_insert_loc > 0) {
            bin.no_hit_insert_loc--;
            evicted.push_back(old);
        } else {
            _set_stats["numHitItemsEvicted"]++;
            _set_stats["sizeHitItemsEvicted"] += old.obj_size;
            if (_cache) {
                _cache->readmitToLogFromSets(old);
            } else {
                evicted.push_back(old);
            }
        }
        bin.items.pop_front();
    }

    if (item.hit_count) {
        item.hit_count = 0;
        bin.items.push_back(item);
    } else {
        assert(bin.no_hit_insert_loc <= bin.items.size());
        auto it = bin.items.begin() + bin.no_hit_insert_loc;
        bin.items.insert(it, item);
        bin.no_hit_insert_loc++;
    }
    bin.bin_size += item.obj_size;
    _total_size += item.obj_size;
    _set_stats["current_size"] = _total_size;
    return evicted;
}

/* this will result in:
 * 1-a: items w/o hits or items not tracked, b-n items w/ hits
 * returns # of first item with a hit
 */
uint64_t Sets::_reorder_set_nru(uint64_t bin_num) {
    assert(bin_num < _num_sets);
    Bin& bin = _bins[bin_num];
    std::vector<bool>& set_hits = _hits_in_sets[bin_num];
    std::list<candidate_t> hit_items;
    std::list<candidate_t> no_hit_items;
    uint64_t i = 0;
    uint64_t size_hits = 0;
    for (auto const& it: bin.items) {
        assert(!(it.obj_size == 0 && it.id == 0));
        if (i >= HIT_BIT_VECTOR_SIZE || !set_hits[i]) {
            no_hit_items.push_back(it);
        } else {
            if (_dist_tracking) {
                size_hits += it.obj_size;
            }
            hit_items.push_back(it);
        }
        i++;
    }
    std::fill(set_hits.begin(), set_hits.end(), false);
    bin.items.clear();
    bin.items.insert(bin.items.end(), no_hit_items.begin(), no_hit_items.end());
    bin.items.insert(bin.items.end(), hit_items.begin(), hit_items.end());
    if (_dist_tracking) {
        std::string num_name = "numItemsWithHits" + std::to_string(hit_items.size());
        size_hits = (size_hits / cache::SIZE_BUCKETING) * cache::SIZE_BUCKETING;
        std::string size_name = "sizeItemsWithHits" + std::to_string(size_hits);
        _set_stats[num_name]++;
        _set_stats[size_name]++;
    }
    return no_hit_items.size();
}

std::vector<candidate_t> Sets::insert(std::vector<candidate_t> items) {
    std::vector<bool> sets_touched(_num_sets, false);
    std::vector<candidate_t> evicted;
    for (auto item: items) {
        uint64_t bin_num = *findSetNums(item).begin();
        assert(bin_num < _num_sets);
        Bin& bin = _bins[bin_num];
        if (!sets_touched[bin_num] && _nru) {
            bin.no_hit_insert_loc = _reorder_set_nru(bin_num);
        } else if (!sets_touched[bin_num]) {
            bin.no_hit_insert_loc = bin.items.size();
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

std::vector<candidate_t> Sets::insert(uint64_t set_num, std::vector<candidate_t> items) {
    std::vector<candidate_t> evicted;
    assert(set_num < _num_sets);
    Bin& bin = _bins[set_num];
    if (_nru) {
        bin.no_hit_insert_loc = _reorder_set_nru(set_num);
    } else {
        bin.no_hit_insert_loc = bin.items.size();
    }
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

void Sets::_updateStatsActualStore(uint64_t num_sets_updated) {
    _set_stats["bytes_written"] += (num_sets_updated * _set_capacity);
}

void Sets::_updateStatsRequestedStore(candidate_t item) {
    _set_stats["stores_requested"]++;
    _set_stats["stores_requested_bytes"] += item.obj_size;
}

bool Sets::find(candidate_t item) {
    std::unordered_set<uint64_t> possible_bins = findSetNums(item);
    for (uint64_t bin_num: possible_bins) {
        uint64_t i = 0;
        for (candidate_t stored_item: _bins[bin_num].items) {
            if (stored_item == item) {
                if (_nru) {
                    _hits_in_sets[bin_num][i] = true;
                }
                if (_hit_dist) {
                    std::string name = "set" + std::to_string(bin_num);
                    _set_stats[name]++;
                }
                _set_stats["hits"]++;
                return true;
            }
            i++;
        }
        if (_hit_dist) {
            std::string name = "setMisses" + std::to_string(bin_num);
            _set_stats[name]++;
        }
    }
    _set_stats["misses"]++;
    return false;
}

std::unordered_set<uint64_t> Sets::findSetNums(candidate_t item) {
    std::unordered_set<uint64_t> possibilities;
    uint64_t current_value = (uint64_t) item.id;
    for (int i = 0; i <= _num_hash_functions; i++) {
        possibilities.insert(current_value % _num_sets);
        current_value = std::hash<std::string>{}(std::to_string(std::hash<uint64_t>{}(current_value)));
    }
    return possibilities;
}

double Sets::ratioCapacityUsed() {
    return _total_size / (double) _total_capacity;
}

double Sets::calcWriteAmp() {
    return _set_stats["bytes_written"]/ (double) _set_stats["stores_requested_bytes"];
}

double Sets::ratioEvictedToCapacity() {
    return _set_stats["sizeEvictions"]/ (double) _total_capacity;
}

void Sets::flushStats() {
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

uint64_t Sets::calcMemoryConsumption() {
    /* TODO: replace 0 w/ bloom filters */
    uint64_t sets_memory_consumption = 0;
    if (_nru) {
        uint64_t bytes_per_set = HIT_BIT_VECTOR_SIZE / 8;
        sets_memory_consumption = bytes_per_set * _num_sets;
    }
    return sets_memory_consumption;
}

bool Sets::trackHit(candidate_t item) {
    std::unordered_set<uint64_t> possible_bins = findSetNums(item);
    for (uint64_t bin_num: possible_bins) {
        uint64_t i = 0;
        for (candidate_t stored_item: _bins[bin_num].items) {
            if (stored_item == item) {
                if (_nru) {
                    _hits_in_sets[bin_num][i] = true;
                }
                _set_stats["hitsSharedWithLog"]++;
                return true;
            }
            i++;
        }
    }
    _set_stats["trackHitsFailed"]++;
    return false;
}

void Sets::enableDistTracking() {
    _dist_tracking = true;
}

void Sets::enableHitDistributionOverSets() {
    _hit_dist = true;
}

} // namespace flashCache

