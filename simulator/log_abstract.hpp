#pragma once

#include <vector>
#include "candidate.hpp"

namespace flashCache {

class LogAbstract {

    public:
        /* ----------- Basic functionality --------------- */
        virtual ~LogAbstract() = default;

        /* insert multiple items (allows amortization of flash write), 
         * no guarantee for placement in multihash */
        virtual std::vector<candidate_t> insert(std::vector<candidate_t> items) = 0;

        /* modified insert, drops item instead of flushing log, assumes wa tally is
         * already counted */
        virtual void insertFromSets(candidate_t item) = 0;
        
        /* returns true if the item is in sets layer */
        virtual bool find(candidate_t item) = 0;
        
        /* ----------- Useful for Admission Policies --------------- */
        
        /* for readmission policies, call on anything dropped before sets */
        virtual void readmit(std::vector<candidate_t> items) = 0;

        /* returns ratio of total capacity used */
        virtual double ratioCapacityUsed() = 0;
        
        /* ----------- Other Bookeeping --------------- */

        virtual double calcWriteAmp() = 0;
        virtual void flushStats() = 0;
};


} // namespace flashCache
