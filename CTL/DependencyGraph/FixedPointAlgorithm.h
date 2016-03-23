#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "AbstractDependencyGraphs.h"
#include "SearchStrategy.h"
#include "Communicator.h"

namespace DependencyGraph {

class FixedPointAlgorithm {
    virtual bool search(BasicDependencyGraph &t_graph,
                        AbstractSearchStrategy &t_W,
                        AbstractSearchStrategy &t_N) =0;
};

class ParallelFixedPointAlgorithm {
    virtual bool search(SerializableDependencyGraph &t_graph,
                        AbstractSearchStrategy &t_W,
                        AbstractSearchStrategy &t_N,
                        AbstractCommunicator &t_channel) =0;
};

}
#endif // FIXEDPOINTALGORITHM_H
