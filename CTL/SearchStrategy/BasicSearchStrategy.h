#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "SearchStrategy.h"
#include "WaitingList.h"
#include <boost/unordered_map.hpp>

namespace SearchStrategy {

class BasicSearchStrategy : public AbstractSearchStrategy {
    using Edge = typename DependencyGraph::Edge;
    using EdgeContainer = EdgeWaitingList<>;

    struct EdgeLessThan {
        bool operator()(Edge* const &l, Edge* const &r) const {
            return l->source->getDistance() < r->source->getDistance();
        }
    };

    //Container for regular edges
    EdgeContainer W;
    //Containers for negation edges
    std::priority_queue<Edge*, std::vector<Edge*>, EdgeLessThan> N;
    //Container for messages
    MessageWaitingList M;


    // AbstractSearchStrategy interface
public:
    enum TaskType {UNAVAILABLE = 0, HYPEREDGE = 1, NEGATIONEDGE = 2, MESSAGE = 3};

    virtual void pushEdge(const DependencyGraph::Edge *edge) { W.push(edge);}
    virtual void pushNegationEdge(const DependencyGraph::Edge *edge) { _N[edge->source->distance].push_back(edge);}
    virtual void pushMessage(const Message &message) {M.push(message);}

    virtual int pickTask(DependencyGraph::Edge &edge,
                         DependencyGraph::Edge &negationEdge,
                         Message &message,
                         int distance) {
        int result = UNAVAILABLE;

        if(distance > 0 && !N.empty()){
            negationEdge = N.top();
            N.pop();
            result = NEGATIONEDGE;
        }
        else if(!M.empty()){
            message = M.top();
            M.pop();
            result = MESSAGE;
        }
        else if(!W.empty()) {
            edge = W.top();
            W.pop();
            result = HYPEREDGE;
        }

        return result;
    }
};
}
#endif // BASICSEARCHSTRATEGY_H
