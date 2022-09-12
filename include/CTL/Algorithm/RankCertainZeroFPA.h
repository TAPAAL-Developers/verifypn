#ifndef RANKCERTAINZEROFPA_H
#define RANKCERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "CTL/PetriNets/PetriConfig.h"
#include "CTL/PetriNets/OnTheFlyDG.h"


namespace Algorithm {

    class RankCertainZeroFPA : public FixedPointAlgorithm {
    public:

        RankCertainZeroFPA(Strategy type);

        virtual ~RankCertainZeroFPA() {
        }
        virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
    protected:
        using wstack_t = std::vector<DependencyGraph::Configuration*>;
        bool _search(DependencyGraph::BasicDependencyGraph &t_graph);
        DependencyGraph::BasicDependencyGraph *graph;
        DependencyGraph::Configuration* root;

        void expand(DependencyGraph::Configuration* conf);
        void push_to_wating(DependencyGraph::Configuration* config, wstack_t& waiting);
        void do_pop(wstack_t& waiting, DependencyGraph::Configuration* lowest = nullptr);
        DependencyGraph::Configuration* find_undecided(DependencyGraph::Configuration* conf);
        DependencyGraph::Configuration* handle_early_czero(DependencyGraph::Configuration* conf);
        void set_assignment(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);

        DependencyGraph::Configuration * backprop(DependencyGraph::Configuration* conf);
        void backprop_edge(DependencyGraph::Edge* conf);
        std::pair<DependencyGraph::Configuration *, DependencyGraph::Assignment>
        eval_edge(DependencyGraph::Edge *e);

        std::string marking(DependencyGraph::Configuration *conf) {
            auto G = dynamic_cast<PetriNets::OnTheFlyDG*> (graph) == nullptr;
            if (!G) return "";
            auto c = static_cast<PetriNets::PetriConfig*> (conf);

        }
        size_t _max_rank = 0;
        bool _early_output = false;
        bool _backloop_output = false;
        Strategy _strategy;
    };
}
#endif // CERTAINZEROFPA_H
