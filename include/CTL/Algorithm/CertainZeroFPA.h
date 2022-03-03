#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "CTL/SearchStrategy/SearchStrategy.h"


namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    CertainZeroFPA(Strategy type) : FixedPointAlgorithm(type)
    {
    }
    virtual ~CertainZeroFPA()
    {
    }
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    DependencyGraph::Configuration* root;

    void checkEdge(DependencyGraph::Edge* e, bool only_assign = false, bool was_dep = false);
    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);



#ifndef NDEBUG
        bool test_invariant(DependencyGraph::Edge *bottom) {
            return true;
        }

#endif
};
}
#endif // CERTAINZEROFPA_H
