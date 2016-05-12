#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "../DependencyGraph/BaiscDependencyGraph.h"
#include "../SearchStrategy/iSearchStrategy.h"
#include "../Communicator/Communicator.h"
#include "PartitionFunction.h"

namespace Algorithm {

class FixedPointAlgorithm {
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iSequantialSearchStrategy &strategy) =0;
};

class DistributedFixedPointAlgorithm {
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iDistributedSearchStrategy &strategy,
                        Communicator &communicator,
                        PartitionFunction &partition_function) =0;
};

}
#endif // FIXEDPOINTALGORITHM_H
