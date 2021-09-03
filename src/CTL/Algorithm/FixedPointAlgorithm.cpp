#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/SearchStrategy/BFSSearch.h"
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/SearchStrategy/HeuristicSearch.h"

namespace CTL {
namespace Algorithm {
    FixedPointAlgorithm::FixedPointAlgorithm(options_t::search_strategy_e type) {
        switch(type)
        {
            case options_t::search_strategy_e::DFS:
                _strategy = std::make_shared<CTL::SearchStrategy::DFSSearch>();
                break;
            case options_t::search_strategy_e::RDFS:
                _strategy = std::make_shared<CTL::SearchStrategy::RDFSSearch>();
                break;
            case options_t::search_strategy_e::BFS:
                _strategy = std::make_shared<CTL::SearchStrategy::BFSSearch>();
                break;
            case options_t::search_strategy_e::HEUR:
                _strategy = std::make_shared<CTL::SearchStrategy::HeuristicSearch>();
                break;
            default:
                assert(false);
                throw base_error(ErrorCode, "Search strategy is unsupported by the CTL-Engine");
        }
    }
}
}
