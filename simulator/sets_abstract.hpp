#pragma once

#include <vector>
#include <unordered_set>
#include "candidate.hpp"
#include "log_abstract.hpp"
#include "stats/stats.hpp"

namespace flashCache {

class SetsAbstract {

    public:
        virtual ~SetsAbstract() = default;

        /* ----------- Basic functionality --------------- */

        /* insert multiple items (allows amortization of flash write), 
         * no guarantee for placement in multihash */
        virtual std::vector<candidate_t> insert(std::vector<candidate_t> items) = 0;
        
        /* 
         * insert multiple items into the same set, useful for multihash
         * will fail assert if any items cannot be matched to provided set_num
         */
        virtual std::vector<candidate_t> insert(uint64_t set_num, std::vector<candidate_t> items) = 0;
        
        /* returns true if the item is in sets layer */
        virtual bool find(candidate_t item) = 0;
        
        /* ----------- Useful for Admission Policies --------------- */

        /* returns the set numbers that item could be mapped to */
        virtual std::unordered_set<uint64_t> findSetNums(candidate_t item) = 0;

        /* returns ratio of total capacity used */
        virtual double ratioCapacityUsed() = 0;

        /* ----------- Other Bookeeping --------------- */

        virtual double calcWriteAmp() = 0;
        virtual double ratioEvictedToCapacity() = 0;
        virtual void flushStats() = 0; // does not print update before clearing
        virtual uint64_t calcMemoryConsumption() = 0;
        virtual bool trackHit(candidate_t item) = 0; // hit on object in sets elsewhere in cache, for nru
        virtual void enableDistTracking() = 0;
        virtual void enableHitDistributionOverSets() = 0;
};


} // namespace flashCache
