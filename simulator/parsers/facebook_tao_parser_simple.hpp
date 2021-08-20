#pragma once

// disable threading as there seems to be a bug in csv.h
#define CSV_IO_NO_THREAD 1

#include <iostream>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <stdint.h>
#include <cassert>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <unordered_set>

#include "../constants.hpp"
#include "../lib/csv.h"

namespace parser {

typedef float float32_t;

class FacebookTaoSimpleParser : public virtual Parser {
public:
  FacebookTaoSimpleParser(std::string filename, uint64_t _numRequests, double _sampling, double _seed, double _object_scaling)
  {
    std::cout << "Parsing simple tao, object file: " << filename << std::endl;

    reader = new FacebookSimpleCSVformat(filename);
    reader->set_header("fbid", "size", "op_count");

    numRequests = _numRequests;
    totalRequests = _numRequests;
    sampling = _sampling;
    scaling = _object_scaling;
    if (sampling != 1) {
        randomGenerator.seed(_seed);
        unif = std::uniform_real_distribution<double>(0.0,1.0);
    }
  }

  void go(VisitorFn visit) {
    std::string tmp;
    size_t op_count = 0;
    int64_t shard = 0;
    int64_t req_size = 0;
    std::string fbid;
    Request req;
    req.req_num = 0;
    while (reader->read_row(fbid, req_size, op_count)) {
      if (numRequests == 0) {
          std::cout << "Finished Processing " 
              << totalRequests << " Requests\n";
          return;
      }


      if (sampling != 1 && (selected_items.find(shard) == selected_items.end())) {
        if (discarded_items.find(shard) == discarded_items.end()) {
            double currentRandom = unif(randomGenerator);
            if (currentRandom < sampling) {
                selected_items.insert(shard);
            } else {
                discarded_items.insert(shard);
                continue;
            }
        } else {
            continue;
        }
      }

      req.id = std::hash<std::string>{}(fbid);
      req.req_size = req_size + SIZE_OF_KEY;
      double s = req.req_size * scaling;
      req.req_size = round(s);
      if (req.req_size >= MAX_TAO_SIZE) {
	req.req_size = MAX_TAO_SIZE - 1;
      } else if (req.req_size == 0) {
          req.req_size = 1;
      }
        
      for (uint i = 0; i < op_count; i++) {
        req.req_num++;
        req.time = req.req_num;
        visit(req);
        if (numRequests != 0) {
            numRequests--;
        }
      }
    }
  }

private:
  typedef io::CSVReader<3, io::trim_chars<' '>, io::no_quote_escape<','>> FacebookSimpleCSVformat;
  FacebookSimpleCSVformat *reader;
  int64_t numRequests;
  int64_t totalRequests;
  double sampling;
  double scaling;
  std::mt19937 randomGenerator;
  std::uniform_real_distribution<double> unif;
  std::unordered_set<int64_t> selected_items;
  std::unordered_set<int64_t> discarded_items;
};

} // namespace parser
