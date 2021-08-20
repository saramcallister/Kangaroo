#pragma once

#include <stdint.h>
#include <limits>

namespace misc {

  // placeholder random number generator from Knuth MMIX
  class Rand {
  public:
    Rand(uint64_t seed = 0) : state(seed) {}

    inline uint64_t next() {
      state = 6364136223846793005 * state + 1442695040888963407;
      return state;
    }

  private:
    uint64_t state;
  };

  template <class RealType = double>
  class UniformDistribution {
  public:
    UniformDistribution(Rand& _rand)
      : rand(_rand) {
    }

    RealType operator() (RealType min=0, RealType max=1) const {
      uint64_t ri = rand.next();
      RealType r = RealType(ri) / std::numeric_limits<uint64_t>::max();
      return ((max - min) * r) + min;
    }

  private:
    Rand& rand;
  };

}
