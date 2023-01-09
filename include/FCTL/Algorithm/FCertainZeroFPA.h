#ifndef FEATURED_CERTAINZEROFPA_H
#define FEATURED_CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "FCTL/DependencyGraph/FConfiguration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "FCTL/SearchStrategy/SearchStrategy.h"

namespace Featured {
namespace Algorithm {

    class FCertainZeroFPA : public FixedPointAlgorithm {
    public:
        FCertainZeroFPA(Strategy type) : FixedPointAlgorithm(type) {
        }

        virtual ~FCertainZeroFPA() {
        }

        virtual bool search(DependencyGraph::BasicDependencyGraph& t_graph) override;

    protected:

        DependencyGraph::BasicDependencyGraph* graph;
        DependencyGraph::Configuration* vertex;

        void checkEdge(DependencyGraph::Edge* e, bool only_assign = false);

        void finalAssign(DependencyGraph::Configuration* c, DependencyGraph::Assignment a);

        void finalAssign(DependencyGraph::Edge* e, DependencyGraph::Assignment a);

        void explore(DependencyGraph::Configuration* c);

    };
}
}
#endif // FEATURED_CERTAINZEROFPA_H
