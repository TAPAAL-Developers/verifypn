/*
 * File:   TARReachability.cpp
 * Author: Peter G. Jensen
 *
 * Created on January 2, 2018, 8:36 AM
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

#include "PetriEngine/TAR/TARReachability.h"
#include "CTL/Stopwatch.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/TAR/AntiChain.h"
#include "PetriEngine/TAR/ContainsVisitor.h"
#include "PetriEngine/TAR/PlaceUseVisitor.h"
#include "PetriEngine/TAR/RangeContext.h"
#include "PetriEngine/TAR/Solver.h"

namespace PetriEngine {
using namespace PQL;
namespace Reachability {

[[maybe_unused]] void TARReachabilitySearch::handle_invalid_trace(trace_t &waiting, int nvalid) {
    // sanity(waiting);
    assert(waiting.size() >= (size_t)nvalid);
    waiting.resize(nvalid); // remove invalid part of trace

    for (size_t i = 1; i < waiting.size(); ++i) {
        bool brk = false;
        for (size_t j = 0; j < i; ++j) {
            if (waiting[j] <= waiting[i]) {
                waiting.resize(i);
                brk = true;
                break;
            }
        }
        if (brk)
            break;
        if (do_step(waiting[i - 1], waiting[i].get_interpolants())) {
            if (i != 1)
                waiting.resize(i - 1);
            break;
        }
    }
    if (waiting.size() == 0)
        return;
}

auto TARReachabilitySearch::pop_done(trace_t &waiting, size_t &stepno) -> bool {
    bool popped = false;
    while (waiting.back().done(_net)) // we have tried all transitions for this state-pair!
    {
        assert(waiting.size() > 0);
        waiting.pop_back();
        popped = true;
        if (waiting.size() > 0) {
            // count up parents edge counter!
            waiting.back().next_edge(_net);
        }
        if (waiting.size() == 0)
            break;
    }
    return popped;
}

void TARReachabilitySearch::next_edge(AntiChain<uint32_t, size_t> &checked, State &state,
                                      trace_t &waiting, std::set<size_t> &nextinter) {
    uint32_t dummy = state.get_edge_cnt() == 0 ? 0 : 1;
    bool res = checked.subsumed(dummy, nextinter);
    if (res) {
        waiting.back().next_edge(_net);
    } else {
        auto minimal = _traceset.minimize(nextinter);
        checked.insert(dummy, minimal);
        State next;
        next.reset_edges(_net);
        next.set_interpolants(nextinter);
        waiting.push_back(next);
    }
}

auto TARReachabilitySearch::run_tar(bool printtrace, Solver &solver, std::vector<bool> &use_trans)
    -> std::pair<bool, bool> {
    Stopwatch tt;
    tt.start();
    auto checked = AntiChain<uint32_t, size_t>();
    // waiting-list with levels
    bool all_covered = true;
    trace_t waiting;
    // initialize
    {
        State state;
        state.reset_edges(_net);
        state.set_interpolants(_traceset.maximize(_traceset.initial()));
        waiting.push_back(state);
    }
    while (!waiting.empty()) {
        if (pop_done(waiting, _stepno))
            continue; // we have reached the end of the edge-iterator for this part of the trace

        ++_stepno;

        assert(waiting.size() > 0);
        State &state = waiting.back();
        std::set<size_t> nextinter;
        if (!use_trans[state.get_edge_cnt()]) {
            state.next_edge(_net);
            continue;
        }
#ifdef TAR_TIMING
        stopwatch ds;
        ds.start();
#endif
        if (do_step(state,
                    nextinter)) // Check if the next state makes the interpolant automata accept.
        {
#ifdef TAR_TIMING
            stopwatch dsn;
            dsn.start();
#endif
            state.next_edge(_net);
#ifdef TAR_TIMING
            dsn.stop();
            _do_step_next += dsn.duration();
            ds.stop();
            _do_step += ds.duration();
#endif
            continue;
        }
#ifdef TAR_TIMING
        ds.stop();
        _do_step += ds.duration();
#endif

        if (waiting.back().get_edge_cnt() == 0) // check if proposition is satisfied
        {
#ifdef TAR_TIMING
            stopwatch ct;
            ct.start();
#endif
            auto satisfied = solver.check(waiting, _traceset);
#ifdef TAR_TIMING
            ct.stop();
            _check_time += ct.duration();
#endif
            if (satisfied) {
                if (printtrace)
                    print_trace(waiting);
                return std::make_pair(true, true);
            } else {
                return std::make_pair(false, false);
                /*                        handleInvalidTrace(waiting, interpolants.size());
                                        all_covered = false;
                                        continue;*/
            }
        } else {
#ifdef VERBOSETAR
            print_stats();
#endif
#ifdef TAR_TIMING
            stopwatch ne;
            ne.start();
#endif
            next_edge(checked, state, waiting, nextinter);
#ifdef TAR_TIMING
            ne.stop();
            _next_time += ne.duration();
#endif
        }
    }
    return std::make_pair(all_covered, false);
}

auto TARReachabilitySearch::try_reach(bool printtrace, Solver &solver) -> bool {
    _traceset.remove_edge(0);
    std::vector<bool> use_trans(_net.number_of_transitions() + 1);
    std::vector<bool> use_place = solver.in_query();
    use_trans[0] = true;
    auto update_use = [&use_trans, &use_place, this](bool any) {
        auto np = use_place;
        bool update = false;
        for (size_t t = 0; t < _net.number_of_transitions(); ++t) {
            auto pre = _net.preset(t);
            auto post = _net.postset(t);
            if (use_trans[t + 1])
                continue;
            {
                for (; pre.first != pre.second; ++pre.first)
                    if (use_place[pre.first->_place])
                        use_trans[t + 1] = true;
                for (; post.first != post.second; ++post.first)
                    if (use_place[post.first->_place])
                        use_trans[t + 1] = true;
            }
            if (use_trans[t + 1]) {
                update = true;
                auto pre = _net.preset(t);
                for (; pre.first != pre.second; ++pre.first)
                    np[pre.first->_place] = true;
                if (any) {
                    std::swap(np, use_place);
                    return true;
                }
            }
        }
        std::swap(np, use_place);
        return update;
    };

    update_use(false);
#ifdef TAR_TIMING
    stopwatch rt;
    rt.start();
#endif
    do {
        auto [finished, satisfied] = run_tar(printtrace, solver, use_trans);
        if (finished) {
            if (!satisfied) {
                if (update_use(false))
                    continue;
#ifdef VERBOSETAR
                for (size_t t = 0; t < _net.number_of_transitions(); ++t) {
                    auto pre = _net.preset(t);
                    std::cerr << "T" << t << "\n";
                    for (; pre.first != pre.second; ++pre.first) {
                        std::cerr << "\tP" << pre.first->place << " - " << pre.first->tokens
                                  << std::endl;
                    }
                    auto post = _net.postset(t);
                    for (; post.first != post.second; ++post.first) {
                        std::cerr << "\tP" << post.first->place << " + " << post.first->tokens
                                  << std::endl;
                    }
                }
                for (size_t p = 0; p < _net.number_of_places(); ++p) {
                    if (_net.initial()[p] != 0)
                        std::cerr << "P" << p << " (" << _net.initial()[p] << ")\n";
                }
#endif
                if (printtrace)
                    _traceset.print(std::cerr);

                // std::vector<size_t>
                // trace{149,123,122,97,157,79,71,24,26,138,3,38,144,143,8,134,6,47,127,11,140,40,27,122};
                //                        std::vector<size_t> trace{
                //                            17,17,17,17,17,56,56,56,56,56,51,6,51,19,24,6,51,40,23,19,24,6,51,40,23,19,24,6,51,40,23,19,24,6,29,24,29,24,29,24,19,23,59,30,36,57,59,30,35,41,56,41,41,59,45,47,40,23,40,23,59,45,47,29,24,29,24,59,45,47,3,5,56,3,5,34,36,57,59,30,35,41,56,41,41,59,45,47,40,23,40,23,59,45,47,40,23,3,5,56,29,24,29,24,29,24,59,45,47,3,5,56
                //                        };
                //                        assert(validate(trace));
                //                        auto [finished, satisfied] = runTAR(printtrace, solver,
                //                        use_trans);
            }
#ifdef TAR_TIMING
            rt.stop();
            std::cerr << "TOT : " << rt.duration()
                      << "\n\tSOLVE : " << (_check_time / rt.duration())
                      << "\n\tSTEP : " << (_do_step / rt.duration())
                      << "\n\tSTEP NEXT : " << (_do_step_next / rt.duration())
                      << "\n\tFOLLOW : " << (_follow_time / rt.duration())
                      << "\n\tNON CHANGE : " << (_non_change_time / rt.duration())
                      << "\n\tNEXT : " << (_next_time / rt.duration()) << std::endl;
#endif
            return satisfied;
        }
    } while (true);
#ifdef TAR_TIMING
    rt.stop();
    std::cerr << "TOT : " << rt.duration() << "\n\tSOLVE : " << (_check_time / rt.duration())
              << "\n\tSTEP : " << (_do_step / rt.duration())
              << "\n\tSTEP NEXT : " << (_do_step_next / rt.duration())
              << "\n\tFOLLOW : " << (_follow_time / rt.duration())
              << "\n\tNON CHANGE : " << (_non_change_time / rt.duration())
              << "\n\tNEXT : " << (_next_time / rt.duration()) << std::endl;
#endif
    return false;
}

auto TARReachabilitySearch::do_step(State &state, std::set<size_t> &nextinter) -> bool {
    // if NFA accepts the trace after this instruction, abort.
#ifdef TAR_TIMING
    stopwatch flw;
    flw.start();
#endif
    if (_traceset.follow(state.get_interpolants(), nextinter, state.get_edge_cnt())) {
#ifdef TAR_TIMING
        flw.stop();
        _follow_time += flw.duration();
#endif
        return true;
    }
#ifdef TAR_TIMING
    flw.stop();
    _follow_time += flw.duration();
#endif
    if (state.get_edge_cnt() == 0)
        return false;
#ifdef TAR_TIMING
    stopwatch nc;
    nc.start();
#endif
    add_non_changing(state, state.get_interpolants(), nextinter);
#ifdef TAR_TIMING
    nc.stop();
    _non_change_time += nc.duration();
#endif
    nextinter = _traceset.maximize(nextinter);
    return false;
}

auto TARReachabilitySearch::validate(const std::vector<size_t> &transitions) -> bool {
    AntiChain<uint32_t, size_t> chain;

    State s;
    s.set_interpolants(_traceset.initial());
    std::cerr << "I ";
    for (auto &i : s.get_interpolants())
        std::cerr << i << ", ";

    std::cerr << std::endl;
    std::set<size_t> next;
    uint32_t dummy = 0;
    chain.insert(dummy, _traceset.minimize(s.get_interpolants()));
    size_t n = 0;
    for (; n < transitions.size(); ++n) {
        auto t = transitions[n];
        s.set_edge(t + 1);
        next.clear();
        if (n == transitions.size() - 1)
            dummy = 0;
        if (do_step(s, next)) {
            std::cerr << "RFAIL AT [" << n << "] = T" << transitions[n] << std::endl;
            return false;
        }
        s.set_interpolants(next);
        if (!chain.insert(dummy, _traceset.minimize(next))) {
            std::cerr << "CFAIL AT [" << n << "] = T" << transitions[n] << std::endl;
        }
    }
    s.set_edge(0);
    if (do_step(s, next)) {
        std::cerr << "FFAIL AT [" << n << "] = T" << transitions[n] << std::endl;
        return false;
    }
    return true;
}

void TARReachabilitySearch::add_non_changing(State &state, std::set<size_t> &maximal,
                                             std::set<size_t> &nextinter) {

    std::vector<int64_t> changes;
    auto pre = _net.preset(state.get_edge_cnt() - 1);
    auto post = _net.postset(state.get_edge_cnt() - 1);

    for (; pre.first != pre.second; ++pre.first) {
        if (pre.first->_inhibitor) {
            assert(false);
            continue;
        }
        changes.push_back(pre.first->_place);
    }

    for (; post.first != post.second; ++post.first) {
        changes.push_back(post.first->_place);
    }
    std::sort(changes.begin(), changes.end());
    _traceset.copy_non_changed(maximal, changes, nextinter);
}

void TARReachabilitySearch::print_trace(trace_t &stack) {
    std::cerr << "Trace:\n<trace>\n";

    _reducer.init_fire(std::cerr);

    for (auto &t : stack) {
        if (t.get_edge_cnt() == 0)
            break;
        std::string tname = _net.transition_names()[t.get_edge_cnt() - 1];
        std::cerr << "\t<transition id=\"" << tname << "\" index=\"" << (t.get_edge_cnt() - 1)
                  << "\">\n";

        // well, yeah, we are not really efficient in constructing the trace.
        // feel free to improve
        auto pre = _net.preset(t.get_edge_cnt() - 1);
        for (; pre.first != pre.second; ++pre.first) {
            for (size_t token = 0; token < pre.first->_tokens; ++token) {
                std::cerr << "\t\t<token place=\"" << _net.place_names()[pre.first->_place]
                          << "\" age=\"0\"/>\n";
            }
        }

        _reducer.extra_consume(std::cerr, tname);

        std::cerr << "\t</transition>\n";

        _reducer.post_fire(std::cerr, tname);
    }

    std::cerr << "</trace>\n" << std::endl;
}

void TARReachabilitySearch::reachable(std::vector<std::shared_ptr<PQL::Condition>> &queries,
                                      std::vector<ResultPrinter::Result> &results, bool printstats,
                                      bool printtrace) {
    // inhibitors are not supported yet
    for (size_t t = 0; t < _net.number_of_transitions(); ++t) {
        auto in = _net.preset(t);
        for (; in.first != in.second; ++in.first) {
            if (in.first->_inhibitor) {
                throw base_error_t("Trace Abstraction Refinement Error : Inhibitor Arcs are not yet "
                                 "supported by the TAR engine");
            }
        }
    }

    // set up working area
    Structures::State state;
    state.set_marking(_net.make_initial_marking());

    // check initial marking
    if (check_queries(queries, results, state, true)) {
        if (printstats)
            print_stats();
        return;
    }

    // Search!
    for (size_t i = 0; i < queries.size(); ++i) {
        _traceset.clear();
        if (results[i] == ResultPrinter::UNKNOWN) {
            PlaceUseVisitor visitor(_net.number_of_places());
            queries[i]->visit(visitor);
            ContainsVisitor<DeadlockCondition> dlvisitor;
            queries[i]->visit(dlvisitor);
            auto used = visitor.in_use();
            if (dlvisitor.does_contain()) {
                for (size_t p = 0; p < _net.number_of_places(); ++p)
                    used[p] = true;
            }
            Solver solver(_net, state.marking(), queries[i].get(), used);
            bool res = try_reach(printtrace, solver);
            if (res)
                results[i] = ResultPrinter::SATISFIED;
            else
                results[i] = ResultPrinter::NOT_SATISFIED;
            auto ret = _printer.handle(i, *queries[i], results[i]);
            results[i] = ret.first;
            if (res && ret.second)
                return;
        }
    }

    if (printstats)
        print_stats();
}

auto TARReachabilitySearch::check_queries(std::vector<std::shared_ptr<PQL::Condition>> &queries,
                                          std::vector<ResultPrinter::Result> &results,
                                          Structures::State &state, bool usequeries) -> bool {
    if (!usequeries)
        return false;

    bool alldone = true;
    for (size_t i = 0; i < queries.size(); ++i) {
        if (results[i] == ResultPrinter::UNKNOWN) {
            EvaluationContext ec(state.marking(), &_net);
            if (queries[i]->evaluate(ec) == Condition::RTRUE) {
                auto ret = _printer.handle(i, *queries[i], ResultPrinter::SATISFIED);
                results[i] = ret.first;
                if (ret.second)
                    return true;
            } else
                alldone = false;
        }
    }
    return alldone;
}

void TARReachabilitySearch::print_stats() {
    std::cerr << "STEPS : " << _stepno << std::endl;
    std::cerr << "INTERPOLANT AUTOMATAS : " << _traceset.initial().size() << std::endl;
}
} // namespace Reachability
} // namespace PetriEngine
