#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "../DependencyGraph/Configuration.h"
#include "FixedPointAlgorithm.h"

namespace CTL::Algorithm {

class LocalFPA : public FixedPointAlgorithm {

    // FixedPointAlgorithm interface
  public:
    LocalFPA(options_t::search_strategy_e type) : FixedPointAlgorithm(type) {}
    ~LocalFPA() override = default;
    auto search(DependencyGraph::BasicDependencyGraph &graph) -> bool override;

  protected:
    DependencyGraph::BasicDependencyGraph *_graph;

    void final_assign(DependencyGraph::Configuration *c, DependencyGraph::assignment_e a);
    void explore(DependencyGraph::Configuration *c);
    void add_dependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target);
};
} // namespace CTL::Algorithm
#endif // LOCALFPA_H
