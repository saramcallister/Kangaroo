#pragma once

#include <vector>

#include "candidate.hpp"

namespace memcache {

class MemCache {
public:

    virtual ~MemCache() = default;

    virtual std::vector<candidate_t> insert(candidate_t id) = 0;

    virtual bool find(candidate_t id) = 0;

    virtual void flushStats() = 0;

};

} // namespace memcache
