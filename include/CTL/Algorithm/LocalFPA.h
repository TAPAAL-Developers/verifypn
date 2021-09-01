#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Configuration.h"

namespace CTL::Algorithm {

class LocalFPA : public FixedPointAlgorithm
{

    // FixedPointAlgorithm interface
public:
    LocalFPA(options_t::SearchStrategy type) : FixedPointAlgorithm(type)
    {
    }
    virtual ~LocalFPA (){}
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph);

protected:
    DependencyGraph::BasicDependencyGraph *_graph;

    void final_assign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void add_dependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);
};
}
#endif // LOCALFPA_H
