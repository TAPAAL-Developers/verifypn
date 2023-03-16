#ifndef FEATURED_FCTLENGINE_H
#define FEATURED_FCTLENGINE_H

#include "utils/errors.h"
#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/options.h"

#include "Algorithm/AlgorithmTypes.h"
#include "../PetriEngine/PQL/PQL.h"

#include "FCTLResult.h"

#include <set>

namespace Featured {
    std::pair<bdd, bdd> FCTLSingleSolve(PetriEngine::PQL::Condition* query, PetriEngine::PetriNet* net,
                         CTL::CTLAlgorithmType algorithmtype,
                         Strategy strategytype, bool partial_order, FCTLResult& result);

    ReturnValue FCTLMain(PetriEngine::PetriNet* net,
                         CTL::CTLAlgorithmType algorithmtype,
                         Strategy strategytype,
                         StatisticsLevel printstatistics,
                         bool partial_order,
                         const std::vector<std::string>& querynames,
                         const std::vector<std::shared_ptr<PetriEngine::PQL::Condition>>& reducedQueries,
                         const std::vector<size_t>& ids,
                         options_t& options);
}
#endif // FCTLENGINE_H
