#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/SearchStrategy/BFSSearch.h"
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/SearchStrategy/HeuristicSearch.h"

namespace CTL {
namespace Algorithm {
    FixedPointAlgorithm::FixedPointAlgorithm(options_t::SearchStrategy type) {
        switch(type)
        {
            case options_t::SearchStrategy::DFS:
                _strategy = std::make_shared<CTL::SearchStrategy::DFSSearch>();
                break;
            case options_t::SearchStrategy::RDFS:
                _strategy = std::make_shared<CTL::SearchStrategy::RDFSSearch>();
                break;
            case options_t::SearchStrategy::BFS:
                _strategy = std::make_shared<CTL::SearchStrategy::BFSSearch>();
                break;
            case options_t::SearchStrategy::HEUR:
                _strategy = std::make_shared<CTL::SearchStrategy::HeuristicSearch>();
                break;
            default:
                assert(false);
                throw base_error(ErrorCode, "Search strategy is unsupported by the CTL-Engine");
        }
    }
}
}
