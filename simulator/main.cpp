#include "caches/cache.hpp"
#include "parsers/parser.hpp"
#include "config.hpp"

using namespace std;

cache::Cache* _cache;

void simulateCache(const parser::Request& req) {
    _cache->access(req);
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: ./cache <config-file>\n");
    exit(-1);
  }

  libconfig::Config cfgFile;

  // Read the file. If there is an error, report it and exit.
  try {
    cfgFile.readFile(argv[1]);
  } catch(const libconfig::FileIOException &fioex) {
    std::cerr << "I/O error while reading config file." << std::endl;
    return(EXIT_FAILURE);
  } catch(const libconfig::ParseException &pex) {
    std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
              << " - " << pex.getError() << std::endl;
    return(EXIT_FAILURE);
  }

  const libconfig::Setting& root = cfgFile.getRoot();
  misc::ConfigReader cfg(root);

  parser::Parser* parserInstance = parser::Parser::create(root);
  _cache = cache::Cache::create(root);
  _cache->dumpStats();

  time_t start = time(NULL);

  parserInstance->go(simulateCache);

  time_t end = time(NULL);

  std::cout << std::endl;
  _cache->dumpStats();

  std::cout << "Processed " << _cache->getTotalAccesses() << " in " 
      << (end - start) << " seconds, rate of " 
      << (1. * _cache->getTotalAccesses() / (end - start)) << " accs/sec" 
      << std::endl;

  delete parserInstance;
  delete _cache;
  return 0;
}
