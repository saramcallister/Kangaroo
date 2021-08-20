#pragma once
#include <limits>

#include "admission/admission.hpp"
#include "rand.hpp"
#include "candidate.hpp"
#include "config.hpp"
#include "stats/stats.hpp"

namespace admission {

class RandomAdmission : public virtual Policy {

public:
    RandomAdmission(const libconfig::Setting& settings, flashCache::SetsAbstract* sets, 
        flashCache::LogAbstract* log, stats::LocalStatsCollector& admission_stats): 
            Policy(admission_stats, sets, log) {
        misc::ConfigReader cfg(settings);
        _admit_ratio = cfg.read<float>("admitRatio");
        _admit_threshold = _admit_ratio * std::numeric_limits<uint64_t>::max();
    }
    
    std::unordered_map<uint64_t, std::vector<candidate_t>> admit(std::vector<candidate_t> items) {
        std::unordered_map<uint64_t, std::vector<candidate_t>> ret = groupBasic(items);
        std::vector<candidate_t> evicted(int(_admit_ratio * items.size()) + 1); 
        for (auto& bin: ret) {
            trackPossibleAdmits(bin.second);
            for (auto it = bin.second.begin(); it != bin.second.end();) {
                if (_rand_gen.next() > _admit_threshold) {
                    evicted.push_back(*it);
                    it = bin.second.erase(it);
                } else {
                    ++it;
                }
            }
            trackAdmitted(bin.second);
        }
        performReadmission(evicted);
        return ret;
    }

    std::vector<candidate_t> admit_simple(std::vector<candidate_t> items) {
        std::vector<candidate_t> admitted;
        admitted.reserve(int(_admit_ratio * items.size()) + 1); 
        trackPossibleAdmits(items);
        for (auto it = items.begin(); it != items.end();) {
            if (_rand_gen.next() <= _admit_threshold) {
                admitted.push_back(*it);
            }
            ++it;
        }
        trackAdmitted(admitted);
        return admitted;
    }

private:
    float _admit_ratio;
    uint64_t _admit_threshold;
    misc::Rand _rand_gen;
};


} // admission namespace
