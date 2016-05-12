#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "../DependencyGraph/AbstractDependencyGraphs.h"
#include "../SearchStrategy/iSearchStrategy.h"

namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iSequantialSearchStrategy &strategy) =0;
};

class DistributedFixedPointAlgorithm {
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iDistributedSearchStrategy &strategy,
                        AbstractCommunicator &channel) =0;
};

}
#endif // FIXEDPOINTALGORITHM_H
