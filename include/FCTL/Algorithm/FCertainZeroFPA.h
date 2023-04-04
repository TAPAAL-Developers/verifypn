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

        virtual std::pair<bdd, bdd> search(DependencyGraph::BasicDependencyGraph& t_graph, bool negate=false) override;

    protected:

        DependencyGraph::BasicDependencyGraph* graph;
        DependencyGraph::Configuration* root;

        void checkEdge(DependencyGraph::Edge* e, bool only_assign = false);

        void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
        void finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);

        void push_dependencies(const DependencyGraph::Configuration* c);
        void explore(DependencyGraph::Configuration* c);

        struct Evaluation {
            DependencyGraph::Configuration* undecided;
            bdd good;
            bdd bad;
        };

        Evaluation evaluate_assignment(DependencyGraph::Edge* e);

        [[nodiscard]] bool try_update(DependencyGraph::Configuration* c, bdd good, bdd bad);
    };
}
}
#endif // FEATURED_CERTAINZEROFPA_H
