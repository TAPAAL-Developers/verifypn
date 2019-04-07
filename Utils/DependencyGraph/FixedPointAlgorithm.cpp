#include "FixedPointAlgorithm.h"
#include "SearchStrategy/BFSSearch.h"
#include "SearchStrategy/DFSSearch.h"
#include "SearchStrategy/RDFSSearch.h"
#include "SearchStrategy/HeuristicSearch.h"
#include "../errorcodes.h"

#include <iostream>

namespace DependencyGraph {
    FixedPointAlgorithm::FixedPointAlgorithm(Utils::SearchStrategies::Strategy type) {
        using namespace Utils::SearchStrategies;
        using namespace SearchStrategy;
        switch(type)
        {
            case DFS:
                strategy = std::make_shared<DFSSearch>();
                break;
            case RDFS:
                strategy = std::make_shared<RDFSSearch>();
                break;
            case BFS:
                strategy = std::make_shared<BFSSearch>();
                break;
            case HEUR:
                strategy = std::make_shared<HeuristicSearch>();
                break;
            default:
                std::cerr << "Search strategy is unsupported by the CTL-Engine"   <<  std::endl;
                assert(false);
                exit(ErrorCode);                
        }
    }


}