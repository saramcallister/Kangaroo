#include <fstream>
#include <iostream>

#include "lib/json.hpp"
#include "stats.hpp"


namespace stats {

LocalStatsCollector::LocalStatsCollector(StatsCollector& parent): 
    _parent(parent) {}

void LocalStatsCollector::print() {
    _parent.print();
}

int64_t &LocalStatsCollector::operator[](std::string name) {
    return counters[name];
}

StatsCollector::StatsCollector(std::string output_filename) {
    std::cout << "Stats file at " << output_filename << std::endl;
    _outputFile.open(output_filename);
}

StatsCollector::~StatsCollector() {
    auto itr = locals.begin();
    while (itr != locals.end()) {
        LocalStatsCollector *local_stats = itr->second;
        itr = locals.erase(itr);
        delete local_stats;
    }
    _outputFile.close();
}

void StatsCollector::print() {
    json blob;
    for (auto local: locals) {
        blob[local.first] = local.second->counters;
    }
    _outputFile << blob.dump(PRETTY_JSON_SPACES) << std::endl;
}

LocalStatsCollector& StatsCollector::createLocalCollector(std::string name) {
    locals[name] = new LocalStatsCollector(*this);
    return *locals[name];
}

} // namespace stats
