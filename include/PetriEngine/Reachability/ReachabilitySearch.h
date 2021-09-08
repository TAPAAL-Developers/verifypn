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
#ifndef BREADTHFIRSTREACHABILITYSEARCH_H
#define BREADTHFIRSTREACHABILITYSEARCH_H

#include "../PQL/PQL.h"
#include "../PetriNet.h"
#include "../ReducingSuccessorGenerator.h"
#include "../Structures/Queue.h"
#include "../Structures/State.h"
#include "../Structures/StateSet.h"
#include "../SuccessorGenerator.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "ReachabilityResult.h"
#include "options.h"

#include <PetriEngine/Stubborn/ReachabilityStubbornSet.h>
#include <memory>
#include <vector>

namespace PetriEngine::Reachability {

/** Implements reachability check in a BFS manner using a hash table */
class ReachabilitySearch {
  public:
    ReachabilitySearch(const PetriNet &net, AbstractHandler &callback, int kbound = 0,
                       bool early = false)
        : _net(net), _kbound(kbound), _callback(callback) {}

    ~ReachabilitySearch() = default;

    /** Perform reachability check using BFS with hasing */
    auto reachable(const std::vector<std::shared_ptr<PQL::Condition>> &queries,
                   std::vector<ResultPrinter::Result> &results,
                   options_t::search_strategy_e strategy, bool usestubborn, bool statespacesearch,
                   bool printstats, bool keep_trace, size_t seed) -> bool;

  private:
    struct searchstate_t {
        size_t _expandedStates = 0;
        size_t _exploredStates = 1;
        std::vector<size_t> _enabledTransitionsCount;
        size_t _heurquery = 0;
        bool _usequeries;
    };

    template <typename Q, typename W = Structures::StateSet, typename G>
    auto try_reach(const std::vector<std::shared_ptr<PQL::Condition>> &queries,
                   std::vector<ResultPrinter::Result> &results, bool usequeries, bool printstats,
                   size_t seed) -> bool;

    template <typename Q> auto init_q(Structures::StateSetInterface &states, size_t seed) -> Q {
        if constexpr (std::is_same<Q, Structures::RDFSQueue>::value)
            return Q(states, seed);
        else
            return Q(states);
    }

    template <typename Q>
    void push_to_q(Q &queue, size_t id, const MarkVal *marking, const PQL::Condition &query) {
        if constexpr (std::is_same<Q, Structures::HeuristicQueue>::value) {
            PQL::DistanceContext dc(_net, marking);
            queue.push(id, query.distance(dc));
        } else
            queue.push(id);
    }

    void handle_completion(const searchstate_t &s, const Structures::StateSetInterface &);
    auto check_queries(const std::vector<std::shared_ptr<PQL::Condition>> &,
                       std::vector<ResultPrinter::Result> &, const Structures::State &,
                       searchstate_t &, const Structures::StateSetInterface &) -> bool;
    auto do_callback(const PQL::Condition &query, size_t i, ResultPrinter::Result r,
                     const searchstate_t &ss, const Structures::StateSetInterface &states)
        -> std::pair<ResultPrinter::Result, bool>;

    const PetriNet &_net;
    int _kbound;
    size_t _satisfyingMarking = 0;
    Structures::State _initial;
    AbstractHandler &_callback;
};

template <typename G>
inline auto make_suc_gen(const PetriNet &net, const std::vector<PQL::Condition_ptr> &queries) -> G {
    return G(net, queries);
}
template <>
inline auto make_suc_gen(const PetriNet &net, const std::vector<PQL::Condition_ptr> &queries)
    -> ReducingSuccessorGenerator {
    auto stubset = std::make_shared<ReachabilityStubbornSet>(net, queries);
    stubset->set_interesting_visitor<InterestingTransitionVisitor>();
    return ReducingSuccessorGenerator{net, stubset};
}

template <typename Q, typename W, typename G>
auto ReachabilitySearch::try_reach(const std::vector<std::shared_ptr<PQL::Condition>> &queries,
                                   std::vector<ResultPrinter::Result> &results, bool usequeries,
                                   bool printstats, size_t seed) -> bool {

    // set up state
    searchstate_t ss;
    ss._enabledTransitionsCount.resize(_net.number_of_transitions(), 0);
    ss._expandedStates = 0;
    ss._exploredStates = 1;
    ss._heurquery = queries.size() >= 2 ? std::rand() % queries.size() : 0;
    ss._usequeries = usequeries;

    // set up working area
    Structures::State state;
    Structures::State working;
    _initial.set_marking(_net.make_initial_marking());
    state.set_marking(_net.make_initial_marking());
    working.set_marking(_net.make_initial_marking());

    W states(_net, _kbound);                      // stateset
    Q queue = init_q<Q>(states, seed);            // working queue
    G generator = make_suc_gen<G>(_net, queries); // successor generator
    auto r = states.add(state);
    // this can fail due to reductions; we push tokens around and violate K
    if (r.first) {
        // add initial to states, check queries on initial state
        _satisfyingMarking = r.second;
        // check initial marking
        if (ss._usequeries) {
            if (check_queries(queries, results, working, ss, states)) {
                if (printstats)
                    handle_completion(ss, states);
                return true;
            }
        }
        // add initial to queue
        push_to_q(queue, r.second, working.marking(), *queries[ss._heurquery]);

        // Search!
        while (queue.pop(state)) {
            generator.prepare(state);

            while (generator.next(working)) {
                ss._enabledTransitionsCount[generator.fired()]++;
                auto res = states.add(working);
                if (res.first) {
                    push_to_q(queue, res.second, working.marking(), *queries[ss._heurquery]);
                    states.set_history(res.second, generator.fired());
                    _satisfyingMarking = res.second;
                    ss._exploredStates++;
                    if (check_queries(queries, results, working, ss, states)) {
                        if (printstats)
                            handle_completion(ss, states);
                        return true;
                    }
                }
            }
            ss._expandedStates++;
        }
    }

    // no more successors, print last results
    for (size_t i = 0; i < queries.size(); ++i) {
        if (results[i] == ResultPrinter::UNKNOWN) {
            results[i] =
                do_callback(*queries[i], i, ResultPrinter::NOT_SATISFIED, ss, states).first;
        }
    }

    if (printstats)
        handle_completion(ss, states);
    return false;
}

} // namespace PetriEngine::Reachability

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
