#ifndef FIXEDPOINTALGORITHM_H
#define FIXEDPOINTALGORITHM_H

#include "DependencyGraph.h"
#include "SearchStrategy/AbstractSearchStrategy.h"

namespace ctl{

class FixedPointAlgorithm
{
public:
    virtual bool search(DependencyGraph &t_graph,
                        AbstractSearchStrategy &t_strategy)=0;
};
}
#endif // FIXEDPOINTALGORITHM_H
