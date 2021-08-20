#pragma once

#include <assert.h>
#include <iostream>
#include <unordered_map>

#include "parsers/parser.hpp"

struct candidate_t {
  int64_t id;
  int64_t obj_size;
  mutable int hit_count;
  int64_t oracle_count;

  static candidate_t make(const parser::Request& req) { 
    return candidate_t{.id=req.id, .obj_size=req.size(), .hit_count=0, .oracle_count=req.oracle_count};
  }

  inline bool operator==(const candidate_t& that) const { 
    return (id == that.id); 
  }

  inline bool operator!=(const candidate_t& that) const { 
    return !(operator==(that)); 
  }

  inline bool operator<(const candidate_t& that) const {
      return this->id < that.id;
  }
};

template <typename T>
class CandidateMap : public std::unordered_map<candidate_t, T> {
public:
  typedef std::unordered_map<candidate_t, T> Base;
  const T DEFAULT;

  CandidateMap(const T& _DEFAULT)
    : Base()
    , DEFAULT(_DEFAULT) {}

  using typename Base::reference;
  using typename Base::const_reference;

  T& operator[] (candidate_t c) { 
    auto itr = Base::find(c);
    if (itr == Base::end()) { 
      auto ret = Base::insert({c, DEFAULT});
      assert(ret.second);
      return ret.first->second;
    }
    else {return itr->second;} 
  }

  const T& operator[] (candidate_t c) const { 
    auto itr = Base::find(c);
    if (itr == Base::end()) { 
      return DEFAULT;
    }
    else {return itr->second;} 
  }
};


// candidate_t specializations
namespace std {

  template <>
  struct hash<candidate_t> {
    size_t operator() (const candidate_t& x) const {
      return x.id;
    }
  };

}

namespace {

  inline std::ostream& operator<< (std::ostream& os, const candidate_t& x) {
    return os << "(" <<  x.id << ")";
  }

}
