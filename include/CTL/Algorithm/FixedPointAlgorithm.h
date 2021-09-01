#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace CTL {
namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph) =0;
    FixedPointAlgorithm(options_t::SearchStrategy type);
    virtual ~FixedPointAlgorithm(){}

    size_t processed_edges() const { return _processedEdges; }
    size_t processed_negation_edges() const { return _processedNegationEdges; }
    size_t explored_configurations() const { return _exploredConfigurations; }
    size_t number_of_edges() const { return _numberOfEdges; }
protected:
    std::shared_ptr<CTL::SearchStrategy::SearchStrategy> _strategy;
    //total number of processed edges
    size_t _processedEdges = 0;
    //total number of processed negation edges
    size_t _processedNegationEdges = 0;
    //number of explored configurations
    size_t _exploredConfigurations = 0;
    //total number of edges found when computing successors
    size_t _numberOfEdges = 0;
};
}
}
#endif // FIXEDPOINTALGORITHM_H
