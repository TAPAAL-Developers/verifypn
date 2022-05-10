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

    bool _search(DependencyGraph::BasicDependencyGraph &t_graph);
    DependencyGraph::BasicDependencyGraph *graph;
    DependencyGraph::Configuration* root;

    void checkEdge(DependencyGraph::Edge* e, bool only_assign = false, bool was_dep = false);
    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);



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
        void backprop(DependencyGraph::Configuration* conf);
        std::pair<DependencyGraph::Configuration *, DependencyGraph::Assignment> eval_edge(DependencyGraph::Edge *e);
    };
}
#endif // CERTAINZEROFPA_H
