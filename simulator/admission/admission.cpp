#include <assert.h>
#include <libconfig.h++>

#include "admission/admission.hpp"
#include "config.hpp"
#include "admission/random_admission.hpp"
#include "admission/threshold.hpp"

namespace admission {

Policy::Policy(stats::LocalStatsCollector& admission_stats,
    flashCache::SetsAbstract* sets, flashCache::LogAbstract* log): 
           _admission_stats(admission_stats), _sets(sets), _log(log) {
}

Policy* Policy::create(const libconfig::Setting& settings, flashCache::SetsAbstract* sets, 
            flashCache::LogAbstract* log, stats::LocalStatsCollector& admission_stats) {
    misc::ConfigReader cfg(settings);

    std::string policyType = cfg.read<const char*>("policy");

    std::cout << "Admission policy: " << policyType << std::endl;
    
    if (policyType == "Random") { 
        return new RandomAdmission(settings, sets, log, admission_stats);
    } else if (policyType == "Threshold") {
        return new Threshold(settings, sets, log, admission_stats);
    } else {
        std::cerr << "Unknown admission policy: " << policyType << std::endl;
        assert(false);
    }

    return NULL;

}

std::unordered_map<uint64_t, std::vector<candidate_t>> Policy::groupBasic(std::vector<candidate_t> items) {
    assert(_sets); // give more helpful error than segfault
    std::unordered_map<uint64_t, std::vector<candidate_t>> grouped;
    for (auto item: items) {
        uint64_t setNum = *(_sets->findSetNums(item).begin());
        grouped[setNum].push_back(item);
    }
    return grouped;
}

void Policy::trackPossibleAdmits(std::vector<candidate_t> items) {
    _admission_stats["trackPossibleAdmitsCalls"]++;
    for (auto item: items) {
        _admission_stats["numPossibleAdmits"]++;
        _admission_stats["sizePossibleAdmits"] += item.obj_size;
    }
}


void Policy::trackAdmitted(std::vector<candidate_t> items) {
    _admission_stats["trackAdmittedCalls"]++;
    for (auto item: items) {
        _admission_stats["numAdmits"]++;
        _admission_stats["sizeAdmits"] += item.obj_size;
    }
}

double Policy::byteRatioAdmitted() {
    return _admission_stats["sizeAdmits"] / (double) _admission_stats["sizePossibleAdmits"];
}

void Policy::performReadmission(std::vector<candidate_t> evicted) {
    if (!_log) {
        /* no readmission possible */
        return;
    } else {
        _log->readmit(evicted);
    }
}

}
