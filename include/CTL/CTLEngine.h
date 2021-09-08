#ifndef CTLENGINE_H
#define CTLENGINE_H

#include "../PetriEngine/PetriNet.h"
#include "../PetriEngine/Reachability/ReachabilitySearch.h"
#include "errorcodes.h"

#include "../PetriEngine/PQL/PQL.h"
#include "Algorithm/AlgorithmTypes.h"
#include "CTLResult.h"

#include <set>

namespace CTL {
auto verify_ctl(const PetriEngine::PetriNet &net, PetriEngine::PQL::Condition_ptr &query,
                options_t &options) -> ctl_result_t;

void print_ctl_result(const std::string &qname, const ctl_result_t &result, size_t index,
                      options_t &options);
} // namespace CTL

#endif // CTLENGINE_H
