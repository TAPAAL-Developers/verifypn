#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "../DependencyGraph/AbstractDependencyGraphs.h"
#include "../SearchStrategy/SearchStrategy.h"
#include "Communicator.h"

namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::AbstractSearchStrategy &strategy) =0;
};
/*
class ParallelFixedPointAlgorithm {
    virtual bool search(DependencyGraph::SerializableDependencyGraph &graph,
                        SearchStrategy::AbstractSearchStrategy &strategy,
                        AbstractCommunicator &channel) =0;
};
*/
}
#endif // FIXEDPOINTALGORITHM_H
