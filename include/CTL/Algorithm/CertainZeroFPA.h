#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "CTL/SearchStrategy/SearchStrategy.h"

namespace CTL {
namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    CertainZeroFPA(options_t::SearchStrategy type) : FixedPointAlgorithm(type)
    {
    }
    virtual ~CertainZeroFPA()
    {
    }
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
protected:

    DependencyGraph::BasicDependencyGraph *_graph;
    DependencyGraph::Configuration* _vertex;
    
    void check_edge(DependencyGraph::Edge* e, bool only_assign = false);
    void final_assign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void final_assign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);

};
}
}
#endif // CERTAINZEROFPA_H
