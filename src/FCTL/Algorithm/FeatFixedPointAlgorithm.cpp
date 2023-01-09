#include "FCTL/Algorithm/FixedPointAlgorithm.h"
#include "FCTL/SearchStrategy/BFSSearch.h"
#include "FCTL/SearchStrategy/DFSSearch.h"
#include "FCTL/SearchStrategy/RDFSSearch.h"
#include "FCTL/SearchStrategy/HeuristicSearch.h"

namespace Featured {
    namespace Algorithm {
        FixedPointAlgorithm::FixedPointAlgorithm(Strategy type) {
            using namespace PetriEngine::Reachability;
            switch (type) {
                case Strategy::DFS:
                    strategy = std::make_shared<DFSSearch>();
                    break;
                case Strategy::RDFS:
                    strategy = std::make_shared<RDFSSearch>();
                    break;
                case Strategy::BFS:
                    strategy = std::make_shared<BFSSearch>();
                    break;
                case Strategy::HEUR:
                    strategy = std::make_shared<HeuristicSearch>();
                    break;
                default:
                    throw base_error("Search strategy is unsupported by the FCTL-Engine");
            }
        }


    }
}