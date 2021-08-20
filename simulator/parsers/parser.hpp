#pragma once

#include <libconfig.h++>

#define SIZE_OF_KEY 20 + 24 //bytes
#define MAX_TAO_SIZE 2048


namespace parser {

struct Request {
  int64_t req_size;
  int64_t id;
  int64_t req_num;
  uint64_t time;
  int32_t type;
  int64_t oracle_count;

  inline int64_t size() const { return req_size; }
} __attribute__((packed));

typedef void (* VisitorFn) (const Request&);

enum {
  GET = 1,
  SET,
  DELETE,
  OTHER
};


class Parser {
    public:
        Parser() {}
        virtual ~Parser() {}
        virtual void go(VisitorFn) = 0;

       static Parser* create(const libconfig::Setting& settings);

};




}
