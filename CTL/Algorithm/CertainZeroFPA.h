#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"

#include <list>
#include <queue>
#include <vector>

namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph,
                        SearchStrategy::AbstractSearchStrategy &t_strategy){};
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::AbstractSearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a){};
    void explore(DependencyGraph::Configuration *c){};
};
}
#endif // CERTAINZEROFPA_H
