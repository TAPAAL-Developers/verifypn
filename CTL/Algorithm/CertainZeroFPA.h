#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"
#include "../SearchStrategy/iSearchStrategy.h"

#include <queue>

namespace Algorithm {

class CertainZeroFPA : public FixedPointAlgorithm
{
    struct edge_prioritizer {
        bool operator()(DependencyGraph::Edge* const &l,
                        DependencyGraph::Edge* const &r) const {
            return l->source->getDistance() > r->source->getDistance();
        }
    };
    using Edge = DependencyGraph::Edge;
    using QueueContainer = std::vector<Edge*>;
    using Queue = std::priority_queue<Edge*, QueueContainer, edge_prioritizer>;

public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph,
                        SearchStrategy::iSequantialSearchStrategy &t_strategy) override;
protected:

    DependencyGraph::BasicDependencyGraph *graph;
    SearchStrategy::iSequantialSearchStrategy *strategy;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);
    void explore(DependencyGraph::Configuration *c);
    void addDependency(DependencyGraph::Edge *e,
                          DependencyGraph::Configuration *target);


    //total number of processed edges
    long s_edges = 0;
    //total number of processed negation edges
    long s_negation_edges = 0;
    //number of explored configurations
    long s_explored = 0;
    //total number of edges found when computing successors
    long s_total_succ = 0;
    //total number of targets found when computing successors
    long s_total_targets = 0;
};
}
#endif // CERTAINZEROFPA_H
