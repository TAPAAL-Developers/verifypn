#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace CTL::Algorithm {

class FixedPointAlgorithm {
  public:
    virtual auto search(DependencyGraph::BasicDependencyGraph &graph) -> bool = 0;
    FixedPointAlgorithm(options_t::search_strategy_e type);
    virtual ~FixedPointAlgorithm() = default;

    [[nodiscard]] auto processed_edges() const -> size_t { return _processedEdges; }
    [[nodiscard]] auto processed_negation_edges() const -> size_t {
        return _processedNegationEdges;
    }
    [[nodiscard]] auto explored_configurations() const -> size_t { return _exploredConfigurations; }
    [[nodiscard]] auto number_of_edges() const -> size_t { return _numberOfEdges; }

  protected:
    std::shared_ptr<CTL::SearchStrategy::SearchStrategy> _strategy;
    // total number of processed edges
    size_t _processedEdges = 0;
    // total number of processed negation edges
    size_t _processedNegationEdges = 0;
    // number of explored configurations
    size_t _exploredConfigurations = 0;
    // total number of edges found when computing successors
    size_t _numberOfEdges = 0;
};
} // namespace CTL::Algorithm
#endif // FIXEDPOINTALGORITHM_H
