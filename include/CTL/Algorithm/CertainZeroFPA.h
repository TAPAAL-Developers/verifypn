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
    CertainZeroFPA(ReachabilityStrategy type) : FixedPointAlgorithm(type)
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
            std::stack<DependencyGraph::Edge*> W;
            std::unordered_set<DependencyGraph::Edge*> passed;

            // check if we would optimize here anyway
            bool allDone = false;
            for (auto e : bottom->source->dependency_set) {
                if (!e->source->isDone()) {

                }
            }

            W.push(bottom);
            while (!W.empty()) {
                auto e = W.top(); W.pop();
                if (std::find(std::begin(passed), std::end(passed), e) == std::end(passed)) {
                    if (e->source->isDone() && e != bottom) {
                        return false;
                    }
                    else {
                        for (auto d : e->source->dependency_set) {
                            if (d->source->isDone()) {
                                return false;
                            }
                            if (d->source == root) {
                                return true;
                            }
                            W.push(d);
                        }
                        passed.insert(e);
                    }
                }
            }
            return true;
        }

#endif
};
}
#endif // CERTAINZEROFPA_H
