/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   Solver.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 3, 2020, 8:08 PM
 */

#ifndef SOLVER_H
#define SOLVER_H

#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "TARAutomata.h"
#include "TraceSet.h"
#include "range.h"

#include <cinttypes>
#include <utility>

namespace PetriEngine::Reachability {
using namespace PQL;
class Solver {
  public:
    using inter_t = std::pair<prvector_t, size_t>;
    using interpolant_t = std::vector<inter_t>;
    Solver(const PetriNet &net, MarkVal *initial, Condition *query, std::vector<bool> &inq);
    auto check(trace_t &trace, TraceSet &interpolants) -> bool;
    [[nodiscard]] auto in_query() const -> const std::vector<bool> & { return _inq; }
    [[nodiscard]] auto query() const -> Condition * { return _query; }

  private:
    auto find_failure(trace_t &trace, bool to_end) -> int64_t;
    auto find_free(trace_t &trace) -> interpolant_t;
    auto compute_hoare(trace_t &trace, interpolant_t &ranges, int64_t fail) -> bool;
    auto compute_terminal(state_t &end, inter_t &last) -> bool;
    const PetriNet &_net;
    MarkVal *_initial;
    Condition *_query;
    std::vector<bool> _inq;
    std::vector<bool> _dirty;
    std::unique_ptr<int64_t[]> _m;
    std::unique_ptr<int64_t[]> _failm;
    std::unique_ptr<MarkVal[]> _mark;
    std::unique_ptr<uint64_t[]> _use_count;
#ifndef NDEBUG
    SuccessorGenerator _gen;
#endif
};
} // namespace PetriEngine::Reachability

#endif /* SOLVER_H */
