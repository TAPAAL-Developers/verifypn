#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "errorcodes.h"
#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/Reachability/ReachabilitySearch.h"

#include "Algorithm/AlgorithmTypes.h"
#include "../PetriEngine/PQL/PQL.h"

#include <set>

namespace CTL {
error_e CTLMain(PetriEngine::PetriNet* net,
            CTL::CTLAlgorithmType algorithmtype,
            options_t::search_strategy_e strategytype,
            bool printstatistics,
            bool mccoutput,
            bool partial_order,
            const std::vector<std::string>& querynames,
            const std::vector<std::shared_ptr<PetriEngine::PQL::Condition>>& reducedQueries,
            const std::vector<size_t>& ids,
            options_t& options);
}

#endif // CTLENGINE_H
