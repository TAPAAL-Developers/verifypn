#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "errorcodes.h"
#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/Reachability/ReachabilitySearch.h"

#include "CTLResult.h"
#include "Algorithm/AlgorithmTypes.h"
#include "../PetriEngine/PQL/PQL.h"

#include <set>

namespace CTL {
    CTLResult verify_ctl(const PetriEngine::PetriNet& net,
            PetriEngine::PQL::Condition_ptr& query,
            options_t& options
        );

    void print_ctl_result(const std::string& qname, const CTLResult& result, size_t index, options_t& options);
}



#endif // CTLENGINE_H
