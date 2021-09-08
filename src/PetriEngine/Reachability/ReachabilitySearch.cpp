/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/SuccessorGenerator.h"

using namespace PetriEngine::PQL;
using namespace PetriEngine::Structures;

namespace PetriEngine::Reachability {

auto ReachabilitySearch::check_queries(const std::vector<std::shared_ptr<PQL::Condition>> &queries,
                                       std::vector<ResultPrinter::Result> &results,
                                       const State &state, searchstate_t &ss,
                                       const StateSetInterface &states) -> bool {
    if (!ss._usequeries)
        return false;

    bool alldone = true;
    for (size_t i = 0; i < queries.size(); ++i) {
        if (results[i] == ResultPrinter::Unknown) {
            EvaluationContext ec(state.marking(), &_net);
            if (queries[i]->evaluate(ec) == Condition::RTRUE) {
                auto r = do_callback(*queries[i], i, ResultPrinter::Satisfied, ss, states);
                results[i] = r.first;
                if (r.second)
                    return true;
            } else {
                alldone = false;
            }
        }
        if (i == ss._heurquery && results[i] != ResultPrinter::Unknown) {
            if (queries.size() >= 2) {
                for (size_t n = 1; n < queries.size(); ++n) {
                    ss._heurquery = (ss._heurquery + n) % queries.size();
                    if (results[i] == ResultPrinter::Unknown)
                        break;
                }
            }
        }
    }
    return alldone;
}

auto ReachabilitySearch::do_callback(const PQL::Condition &query, size_t i, ResultPrinter::Result r,
                                     const searchstate_t &ss,
                                     const Structures::StateSetInterface &states)
    -> std::pair<ResultPrinter::Result, bool> {
    return _callback.handle(i, query, r, &states.max_place_bound(), ss._expandedStates,
                            ss._exploredStates, states.discovered(), states.max_tokens(), &states,
                            _satisfyingMarking, _initial.marking());
}

void ReachabilitySearch::handle_completion(const searchstate_t &ss,
                                           const Structures::StateSetInterface &states) {
    std::cout << "STATS:\n"
              << "\tdiscovered states: " << states.discovered() << std::endl
              << "\texplored states:   " << ss._exploredStates << std::endl
              << "\texpanded states:   " << ss._expandedStates << std::endl
              << "\tmax tokens:        " << states.max_tokens() << std::endl;

    std::cout << "\nTRANSITION STATISTICS\n";
    for (size_t i = 0; i < _net.number_of_transitions(); ++i) {
        std::cout << "<" << _net.transition_names()[i] << ":" << ss._enabledTransitionsCount[i]
                  << ">";
    }
    // report how many times transitions were enabled (? means that the transition was removed in
    // net reduction)
    for (size_t i = _net.number_of_transitions(); i < _net.transition_names().size(); ++i) {
        std::cout << "<" << _net.transition_names()[i] << ":?>";
    }

    std::cout << "\n\nPLACE-BOUND STATISTICS\n";
    for (size_t i = 0; i < _net.number_of_places(); ++i) {
        std::cout << "<" << _net.place_names()[i] << ";" << states.max_place_bound()[i] << ">";
    }

    // report maximum bounds for each place (? means that the place was removed in net reduction)
    for (size_t i = _net.number_of_places(); i < _net.place_names().size(); ++i) {
        std::cout << "<" << _net.place_names()[i] << ";?>";
    }

    std::cout << std::endl << std::endl;
}

#define TRYREACHPAR (queries, results, usequeries, printstats, seed)
#define TEMPPAR(X, Y)                                                                              \
    if (keep_trace)                                                                                \
        return try_reach<X, Structures::TracableStateSet, Y> TRYREACHPAR;                          \
    else                                                                                           \
        return try_reach<X, Structures::StateSet, Y> TRYREACHPAR;
#define TRYREACH(X)                                                                                \
    if (stubbornreduction)                                                                         \
        TEMPPAR(X, ReducingSuccessorGenerator)                                                     \
    else                                                                                           \
        TEMPPAR(X, SuccessorGenerator)

auto ReachabilitySearch::reachable(const std::vector<std::shared_ptr<PQL::Condition>> &queries,
                                   std::vector<ResultPrinter::Result> &results,
                                   options_t::search_strategy_e strategy, bool stubbornreduction,
                                   bool statespacesearch, bool printstats, bool keep_trace,
                                   size_t seed) -> bool {
    bool usequeries = !statespacesearch;

    // if we are searching for bounds
    if (!usequeries)
        strategy = options_t::search_strategy_e::BFS;

    switch (strategy) {
    case options_t::search_strategy_e::DFS:
        TRYREACH(DFSQueue)
        break;
    case options_t::search_strategy_e::BFS:
        TRYREACH(BFSQueue)
        break;
    case options_t::search_strategy_e::DEFAULT:
    case options_t::search_strategy_e::HEUR:
        TRYREACH(HeuristicQueue)
        break;
    case options_t::search_strategy_e::RDFS:
        TRYREACH(RDFSQueue)
        break;
    default:
        throw base_error_t("Unsupported search strategy for ReachabilitySearch");
    }
}
} // namespace PetriEngine::Reachability
