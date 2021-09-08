#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "FixedPointAlgorithm.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace CTL::Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm {
  public:
    CertainZeroFPA(options_t::search_strategy_e type) : FixedPointAlgorithm(type) {}
    ~CertainZeroFPA() override = default;
    auto search(DependencyGraph::BasicDependencyGraph &t_graph) -> bool override;

  protected:
    DependencyGraph::BasicDependencyGraph *_graph;
    DependencyGraph::Configuration *_vertex;

    void check_edge(DependencyGraph::Edge *e, bool only_assign = false);
    void final_assign(DependencyGraph::Configuration *c, DependencyGraph::assignment_e a);
    void final_assign(DependencyGraph::Edge *e, DependencyGraph::assignment_e a);
    void explore(DependencyGraph::Configuration *c);
};
} // namespace CTL::Algorithm
#endif // CERTAINZEROFPA_H
