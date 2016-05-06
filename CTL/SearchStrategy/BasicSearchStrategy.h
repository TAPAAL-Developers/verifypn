#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "SearchStrategy.h"
#include "WaitingList.h"
#include <boost/unordered_map.hpp>

#include <iostream>

namespace SearchStrategy {

class BasicSearchStrategy : public AbstractSearchStrategy {
    using Edge = DependencyGraph::Edge;
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

    virtual bool empty() { return W.empty() && N.empty() && M.empty(); }

    virtual void pushEdge(DependencyGraph::Edge *edge) { W.push(edge);}
    virtual void pushNegationEdge(DependencyGraph::Edge *edge) { N.push(edge);}
    virtual void pushMessage(Message &message) {M.push(message);}

    virtual int pickTask(DependencyGraph::Edge *&edge,
                         DependencyGraph::Edge *&negationEdge,
                         Message *&message,
                         int distance) {
        int result = M.empty() && N.empty() && W.empty() ? EMPTY : UNAVAILABLE;;

        std::cout << "Distance: " << distance << std::endl;

        if(distance > 0 && !N.empty()){
            negationEdge = N.top();
            negationEdge->source->printConfiguration();
            N.pop();
            result = NEGATIONEDGE;
        }
        else if(!M.empty()){
            *message = M.top();
            std::cout << message->ToString() << std::endl;
            M.pop();
            result = MESSAGE;
        }
        else if(!W.empty()) {
            edge = W.top();
            edge->source->printConfiguration();
            W.pop();
            result = HYPEREDGE;
        }

        return result;
    }
};
}
#endif // BASICSEARCHSTRATEGY_H
