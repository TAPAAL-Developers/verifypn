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

namespace PetriEngine {
namespace Reachability {
using namespace PQL;
class Solver {
  public:
    using inter_t = std::pair<prvector_t, size_t>;
    using interpolant_t = std::vector<inter_t>;
    Solver(const PetriNet &net, MarkVal *initial, Condition *query, std::vector<bool> &inq);
    bool check(trace_t &trace, TraceSet &interpolants);
    const std::vector<bool> &in_query() const { return _inq; }
    Condition *query() const { return _query; }

  private:
    int64_t find_failure(trace_t &trace, bool to_end);
    interpolant_t find_free(trace_t &trace);
    bool compute_hoare(trace_t &trace, interpolant_t &ranges, int64_t fail);
    bool compute_terminal(state_t &end, inter_t &last);
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
} // namespace Reachability
} // namespace PetriEngine

#endif /* SOLVER_H */
