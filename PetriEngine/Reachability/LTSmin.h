#ifndef LTSMIN_H
#define LTSMIN_H

#include "../PetriNet.h"
#include "ReachabilityResult.h"

namespace PetriEngine { namespace Reachability {

class LTSmin {
public:
    LTSmin(){

    }

    ReachabilityResult reachable(std::string cmd, int xmlquery, std::string queryId, bool isPlaceBound, bool isReachBound);

private:
    std::string cmd;

    std::string searchExit;
    std::string searchPins2lts;  
};

} // Reachability
} // PetriEngine

#endif // LTSMIN_H
