#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph) =0;
    FixedPointAlgorithm(Strategy type);
    virtual ~FixedPointAlgorithm(){}

    size_t processedEdges() const { return _processedEdges; }
    size_t processedNegationEdges() const { return _processedNegationEdges; }
    size_t exploredConfigurations() const { return _exploredConfigurations; }
    size_t numberOfEdges() const { return _numberOfEdges; }
    size_t _optim_procs = 0;
    std::shared_ptr<SearchStrategy::SearchStrategy> strategy;
protected:
    //total number of processed edges
    size_t _processedEdges = 0;
    //total number of processed negation edges
    size_t _processedNegationEdges = 0;
    //number of explored configurations
    size_t _exploredConfigurations = 0;
    //total number of edges found when computing successors
    size_t _numberOfEdges = 0;
    // number of times the rank optimization was applied
    size_t _num_rank_checks = 0;
    size_t _cycle_count = 0;
    size_t _cycle_assignments = 0;
};
}
#endif // FIXEDPOINTALGORITHM_H
