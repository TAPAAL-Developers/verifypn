#ifndef BASICDISTSTRATEGY_H
#define BASICDISTSTRATEGY_H


#include "iSearchStrategy.h"
#include "../DependencyGraph/Configuration.h"

#include "NegationWaitingList.h"

#include <stack>
#include <queue>



namespace SearchStrategy {

class BasicDistStrategy : public iDistributedSearchStrategy
{
public:
    BasicDistStrategy();

//dist search strategy interface
    virtual bool empty() const override;
    virtual unsigned int maxDistance() const override;
    virtual void pushEdge(DependencyGraph::Edge *edge) override;
    virtual void pushMessage(Message &message) override;
    virtual void releaseNegationEdges(int dist) override;

    virtual TaskType pickTask(DependencyGraph::Edge*& edge,
                              Message& message) override;

protected:
/*
    //Negation priority queue
    struct edge_prioritizer{
        //return true if left < right (right should be picked first)
        bool operator()(const DependencyGraph::Edge *lhs, const DependencyGraph::Edge *rhs) const {
            return (lhs->source->getDistance() <= rhs->source->getDistance());
        }
    };
    typedef std::priority_queue<DependencyGraph::Edge*, std::vector<DependencyGraph::Edge*>, SearchStrategy::BasicDistStrategy::edge_prioritizer> PriorityQueue;
*/

    NegationWaitingList negation_edges;

    std::stack<Message> messages;
    std::stack<DependencyGraph::Edge*> edges;

};

}

#endif // BASICDISTSTRATEGY_H
