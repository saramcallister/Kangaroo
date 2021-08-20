#include <libconfig.h++>

#include "parser.hpp"
#include "../config.hpp"
#include "facebook_tao_parser_simple.hpp"
#include "zipf_parser.hpp"

parser::Parser* parser::Parser::create(const libconfig::Setting& settings) {
    misc::ConfigReader cfg(settings);

    std::string parserType = cfg.read<const char*>("trace.format");

    std::cout << "Parser type: " << parserType << std::endl;
    
    auto numKRequests = cfg.read<int>("trace.totalKAccesses", -1);
    int64_t numRequests = 1024 * numKRequests;
    std::cout << "num requests: " << numRequests << std::endl;

    if (parserType == "Zipf") { 
        assert(numRequests > 0);
        auto alpha = cfg.read<float>("trace.alpha");
        auto numObjects = cfg.read<int>("trace.numObjects");
        return new ZipfParser(alpha, numObjects, numRequests);
    } else if (parserType == "FacebookTaoSimple") {
        std::string filename1 = cfg.read<const char *>("trace.filename");
        double sampling = cfg.read<double>("trace.samplingPercent", 1);
        int seed = cfg.read<int>("trace.samplingSeed", 0);
        double scaling = cfg.read<double>("trace.objectScaling", 1);
        return new FacebookTaoSimpleParser(filename1, numRequests, sampling, seed, scaling);
    } else {
        std::cerr << "Unknown parser type: " << parserType << std::endl;
        assert(false);
    }

}
