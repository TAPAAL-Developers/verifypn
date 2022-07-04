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

class RankCertainZeroFPA : public FixedPointAlgorithm
{
public:
    RankCertainZeroFPA(Strategy type) : FixedPointAlgorithm(type)
    {
    }
    virtual ~RankCertainZeroFPA()
    {
    }
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph) override;
protected:
    using wstack_t = std::vector<DependencyGraph::Configuration*>;
    bool _search(DependencyGraph::BasicDependencyGraph &t_graph);
    DependencyGraph::BasicDependencyGraph *graph;
    DependencyGraph::Configuration* root;

    void checkEdge(DependencyGraph::Edge* e, bool only_assign = false, bool was_dep = false);
    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);
    void set_assignment(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    std::vector<DependencyGraph::Edge*> explore(DependencyGraph::Configuration *c);



//#ifndef NDEBUG
        bool test_invariant(DependencyGraph::Edge *e) {
            bool inv_good = false;
            if (e->source != root) {
                std::stack<DependencyGraph::Edge*> W;
                std::unordered_set<DependencyGraph::Edge*> passed;

                W.push(e);
                while (!W.empty()) {
                    auto edge = W.top(); W.pop();
                    if (std::find(std::begin(passed), std::end(passed), edge) == std::end(passed)) {
                        if (edge->source->isDone() && edge != e) {
                            continue;
                        }
                        else {
                            for (auto d : edge->source->dependency_set) {
                                if (d->source == root) {
                                    inv_good = true; break;
                                }
                                if (!d->source->isDone()) {
                                    W.push(d);
                                }
                            }
                            passed.insert(edge);
                        }
                    }
                }
            }
            else return true;
            return inv_good;
        }

//#endif
        DependencyGraph::Configuration * backprop(DependencyGraph::Configuration* conf);
        void backprop_edge(DependencyGraph::Edge* conf);
        std::pair<DependencyGraph::Configuration *, DependencyGraph::Assignment>
        eval_edge(DependencyGraph::Edge *e, wstack_t* waiting = nullptr);


        std::string marking(DependencyGraph::Configuration *conf) {
            auto G = dynamic_cast<PetriNets::OnTheFlyDG*>(graph) == nullptr;
            if (!G) return "";
            auto c = static_cast<PetriNets::PetriConfig*>(conf);

        }
        size_t _max_rank = 0;
    };
}
#endif // CERTAINZEROFPA_H
