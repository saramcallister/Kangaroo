#pragma once

#include <fstream>
#include <memory>
#include <unordered_map>

#include "lib/json.hpp"

#define PRETTY_JSON_SPACES 4

namespace stats {

using json = nlohmann::json;

class StatsCollector;

class LocalStatsCollector {
    public:
        int64_t &operator[](std::string name);
        void print();
        friend class StatsCollector;
        ~LocalStatsCollector() {}

    private:
        std::unordered_map<std::string, int64_t> counters;
        LocalStatsCollector(StatsCollector& parent);
        StatsCollector& _parent;
};

class StatsCollector {
    public:
        StatsCollector(std::string output_filename);
        ~StatsCollector();
        LocalStatsCollector& createLocalCollector(std::string name);
        void print();

    private:
        std::ofstream _outputFile;
        std::unordered_map<std::string, LocalStatsCollector*> locals;
};


} // namespace stats
