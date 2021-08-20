#include <cmath>
#include <libconfig.h++>

#include "config.hpp"
#include "constants.hpp"
#include "caches/set_only_cache.hpp"
#include "caches/mem_only_cache.hpp"
#include "caches/mem_log_cache.hpp"
#include "caches/mem_log_sets_cache.hpp"
#include "caches/cache.hpp"


namespace cache {

Cache::Cache(stats::StatsCollector* sc, stats::LocalStatsCollector& gs, const libconfig::Setting &settings):
        statsCollector(sc), globalStats(gs), _historyAccess(false) {
    misc::ConfigReader cfg(settings);
    int stats_power = cfg.read<int>("stats.collectionIntervalPower", STATS_INTERVAL_POWER);
    _stats_interval = pow(10, stats_power);
}

Cache::~Cache() {
    delete statsCollector;
}

Cache* Cache::create(const libconfig::Setting &settings) {
    misc::ConfigReader cfg(settings);

    Cache* cache_ret;
  
    /* initialize stats collection */
    std::string filename = cfg.read<const char*>("stats.outputFile");
    auto sc = new stats::StatsCollector(filename);
    auto& gs = sc->createLocalCollector("global");

    /* determine components */
    bool memoryCache = cfg.exists("memoryCache");
    bool log = cfg.exists("log");
    bool sets = cfg.exists("sets");

    if (memoryCache && !log && sets) {
        cache_ret = new SetOnlyCache(sc, gs, settings);
    } else if (memoryCache && log && sets) {
        cache_ret = new MemLogSetsCache(sc, gs, settings);
    } else if (memoryCache && log && !sets) {
        cache_ret = new MemLogCache(sc, gs, settings);
    } else if (memoryCache && !log && !sets) {
        cache_ret = new MemOnlyCache(sc, gs, settings);
    } else {
        std::cerr << "No appropriate cache implementation for: \n" 
            << "\tMemeory Cache: " << memoryCache << "\n"
            << "\tLog: " << log << "\n"
            << "\tSets: " << sets 
            << std::endl;
        assert(false);
    }

    return cache_ret;
}

void Cache::access(const parser::Request& req) {
    assert(req.size() >= 0);
    if (req.type >= parser::SET) { return; }
    globalStats["timestamp"] = req.time;

    auto id = candidate_t::make(req);
    bool hit = this->find(id);
    
    if (hit) {
        globalStats["hits"]++;
        globalStats["hitsSize"] += id.obj_size;
    } else {
        globalStats["misses"]++;
        globalStats["missesSize"] += id.obj_size;
    }

    trackAccesses();
    trackHistory(id);

    // stats?
    if ((_stats_interval > 0) && ((getTotalAccesses() % _stats_interval) == 0)) {
        dumpStats();
    }

    if (!hit) {
        this->insert(id);
    }
}
    

void Cache::dumpStats() {
    std::cout << "Miss Rate: " << calcMissRate() 
            << " Flash Write Amp: " << calcFlashWriteAmp() << std::endl;
    statsCollector->print();
}

uint64_t Cache::getTotalAccesses() {
    return globalStats["totalAccesses"];
}

uint64_t Cache::getAccessesAfterFlush() {
    return globalStats["accessesAfterFlush"];
}

void Cache::trackAccesses() {
    globalStats["totalAccesses"]++;
    globalStats["accessesAfterFlush"]++;
}

void Cache::trackHistory(candidate_t id) {
    if (!_historyAccess[id]) {
        // first time requests are considered as compulsory misses
        globalStats["compulsoryMisses"]++;
        _historyAccess[id] = true;
        globalStats["uniqueBytes"] += id.obj_size;
    } 
}

double Cache::calcMissRate() {
    return globalStats["misses"] / (double) getAccessesAfterFlush();
}

void Cache::flushStats() {
    dumpStats();
    globalStats["hits"] = 0;
    globalStats["misses"] = 0;
    globalStats["hitsSize"] = 0;
    globalStats["missesSize"] = 0;
    globalStats["accessesAfterFlush"] = 0;
    globalStats["compulsoryMisses"] = 0;
    globalStats["numStatFlushes"]++;
}

} // namespace Cache 


