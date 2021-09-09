/*
 * File:   TARReachability.h
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

#ifndef TARREACHABILITY_H
#define TARREACHABILITY_H
#include "AntiChain.h"
#include "TARAutomata.h"
#include "TraceSet.h"

#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace PetriEngine::Reachability {
class Solver;
class TARReachabilitySearch {
  private:
    const AbstractHandler &_printer;

  public:
    TARReachabilitySearch(const AbstractHandler &printer, const PetriNet &net,
                          const Reducer &reducer, int kbound = 0)
        : _printer(printer), _net(net), _reducer(reducer), _traceset(net) {
        _kbound = kbound;
    }

    ~TARReachabilitySearch() = default;

    void reachable(std::vector<std::shared_ptr<PQL::Condition>> &queries,
                   std::vector<ResultPrinter::result_e> &results, bool printstats, bool printtrace);

  private:
    void print_trace(trace_t &stack);
    void next_edge(AntiChain<uint32_t, size_t> &checked, State &state, trace_t &waiting,
                   std::set<size_t> &nextinter);
    auto try_reach(bool printtrace, Solver &solver) -> bool;
    auto run_tar(bool printtrace, Solver &solver, std::vector<bool> &use_trans)
        -> std::pair<bool, bool>;
    auto pop_done(trace_t &waiting, size_t &stepno) -> bool;
    auto do_step(State &state, std::set<size_t> &nextinter) -> bool;
    void add_non_changing(State &state, std::set<size_t> &maximal, std::set<size_t> &nextinter);
    auto validate(const std::vector<size_t> &transitions) -> bool;

    [[maybe_unused]] void handle_invalid_trace(trace_t &waiting, int nvalid);
    auto is_valid_trace(trace_t &trace, Structures::State &initial, const std::vector<bool> &,
                        PQL::Condition *query) -> std::pair<int, bool>;
    void print_stats();
    auto check_queries(std::vector<std::shared_ptr<PQL::Condition>> &,
                       std::vector<ResultPrinter::result_e> &, Structures::State &, bool) -> bool;

    int _kbound;
    size_t _stepno = 0;
    const PetriNet &_net;
    const Reducer &_reducer;
    TraceSet _traceset;

#ifdef TAR_TIMING
    double _check_time = 0;
    double _next_time = 0;
    double _do_step = 0;
    double _do_step_next = 0;
    double _non_change_time = 0;
    double _follow_time = 0;
#endif
};

} // namespace PetriEngine::Reachability
#endif /* TARREACHABILITY_H */
