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

#include "../Structures/State.h"
#include "ReachabilityResult.h"
#include "../PQL/PQL.h"
#include "../PetriNet.h"
#include "../Structures/StateSet.h"
#include "../Structures/Queue.h"
#include "../SuccessorGenerator.h"
#include "../ReducingSuccessorGenerator.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "options.h"

#include <memory>
#include <vector>
#include <PetriEngine/Stubborn/ReachabilityStubbornSet.h>


namespace PetriEngine {
    namespace Reachability {

        /** Implements reachability check in a BFS manner using a hash table */
        class ReachabilitySearch {
        public:

            ReachabilitySearch(const PetriNet& net, AbstractHandler& callback, int kbound = 0, bool early = false)
            : _net(net), _kbound(kbound), _callback(callback) {
            }

            ~ReachabilitySearch()
            {
            }

            /** Perform reachability check using BFS with hasing */
            bool reachable(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    options_t::search_strategy_e strategy,
                    bool usestubborn,
                    bool statespacesearch,
                    bool printstats,
                    bool keep_trace,
                    size_t seed);
        private:
            struct searchstate_t {
                size_t _expandedStates = 0;
                size_t _exploredStates = 1;
                std::vector<size_t> _enabledTransitionsCount;
                size_t _heurquery = 0;
                bool _usequeries;
            };

            template<typename Q, typename W = Structures::StateSet, typename G>
            bool try_reach(
                std::vector<std::shared_ptr<PQL::Condition > >& queries,
                std::vector<ResultPrinter::Result>& results,
                bool usequeries,
                bool printstats,
                size_t seed);
            void print_stats(searchstate_t& s, Structures::StateSetInterface*);
            bool check_queries(  std::vector<std::shared_ptr<PQL::Condition > >&,
                                    std::vector<ResultPrinter::Result>&,
                                    Structures::State&, searchstate_t&, Structures::StateSetInterface*);
            std::pair<ResultPrinter::Result,bool> do_callback(std::shared_ptr<PQL::Condition>& query, size_t i, ResultPrinter::Result r, searchstate_t &ss, Structures::StateSetInterface *states);

            const PetriNet& _net;
            int _kbound;
            size_t _satisfyingMarking = 0;
            Structures::State _initial;
            AbstractHandler& _callback;
        };

        template <typename G>
        inline G _make_suc_gen(const PetriNet &net, std::vector<PQL::Condition_ptr> &queries) {
            return G{net, queries};
        }
        template <>
        inline ReducingSuccessorGenerator _make_suc_gen(const PetriNet &net, std::vector<PQL::Condition_ptr> &queries) {
            auto stubset = std::make_shared<ReachabilityStubbornSet>(net, queries);
            stubset->set_interesting_visitor<InterestingTransitionVisitor>();
            return ReducingSuccessorGenerator{net, stubset};
        }

        template<typename Q, typename W, typename G>
        bool ReachabilitySearch::try_reach(   std::vector<std::shared_ptr<PQL::Condition> >& queries,
                                        std::vector<ResultPrinter::Result>& results, bool usequeries,
                                        bool printstats, size_t seed)
        {

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

            W states(_net, _kbound);    // stateset
            Q queue(&states, seed);           // working queue
            G generator = _make_suc_gen<G>(_net, queries); // successor generator
            auto r = states.add(state);
            // this can fail due to reductions; we push tokens around and violate K
            if(r.first){
                // add initial to states, check queries on initial state
                _satisfyingMarking = r.second;
                // check initial marking
                if(ss._usequeries)
                {
                    if(check_queries(queries, results, working, ss, &states))
                    {
                        if(printstats) print_stats(ss, &states);
                            return true;
                    }
                }
                // add initial to queue
                {
                    PQL::DistanceContext dc(_net, working.marking());
                    queue.push(r.second, dc, queries[ss._heurquery]);
                }

                // Search!
                while (queue.pop(state)) {
                    generator.prepare(state);

                    while(generator.next(working)){
                        ss._enabledTransitionsCount[generator.fired()]++;
                        auto res = states.add(working);
                        if (res.first) {
                            {
                                PQL::DistanceContext dc(_net, working.marking());
                                queue.push(res.second, dc, queries[ss._heurquery]);
                            }
                            states.set_history(res.second, generator.fired());
                            _satisfyingMarking = res.second;
                            ss._exploredStates++;
                            if (check_queries(queries, results, working, ss, &states)) {
                                if(printstats) print_stats(ss, &states);
                                return true;
                            }
                        }
                    }
                    ss._expandedStates++;
                }
            }

            // no more successors, print last results
            for(size_t i= 0; i < queries.size(); ++i)
            {
                if(results[i] == ResultPrinter::Unknown)
                {
                    results[i] = do_callback(queries[i], i, ResultPrinter::NotSatisfied, ss, &states).first;
                }
            }

            if(printstats) print_stats(ss, &states);
            return false;
        }


    }
} // Namespaces

#endif // BREADTHFIRSTREACHABILITYSEARCH_H
