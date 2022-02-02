#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "CTL/DependencyGraph/CTLHeuristicVisitor.h"

namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    CertainZeroFPA(Strategy type)
            : FixedPointAlgorithm(type),
              _heuristic(CTLHeuristicVisitor::JiriVal)
    {
    }
    virtual ~CertainZeroFPA()
    {
    }
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    DependencyGraph::Configuration* vertex;

    void checkEdge(DependencyGraph::Edge* e, bool only_assign = false);
    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);

private:
    CTLHeuristicVisitor _heuristic;

    void _order_successors(std::vector<DependencyGraph::Edge*> &sucs);
};
}
#endif // CERTAINZEROFPA_H
