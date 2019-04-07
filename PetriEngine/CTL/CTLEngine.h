#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/options.h"

#include "Utils/DependencyGraph/AlgorithmTypes.h"
#include "Utils/SearhStrategies.h"
#include "Utils/errorcodes.h"

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    DependencyGraph::AlgorithmType algorithmtype,
                    Utils::SearchStrategies::Strategy strategytype,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput,
                    bool partial_order,
                    const std::vector<std::string>& querynames,
                    const std::vector<std::shared_ptr<PetriEngine::PQL::Condition>>& reducedQueries,
                    const std::vector<size_t>& ids,
                    options_t& options);

#endif // CTLENGINE_H
