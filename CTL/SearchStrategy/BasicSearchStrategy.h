#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "SearchStrategy.h"

namespace SearchStrategy {

template<class edgeWaitingList,
         class negationWaitingList,
         class messageWaitingList>
class BasicSearchStrategy : public AbstractSearchStrategy {


    // AbstractSearchStrategy interface
public:
    virtual void pushEdge(const DependencyGraph::Edge *edge);
    virtual void pushNegationEdge(const DependencyGraph::Edge *edge);
    virtual void pushMessage(const Message &message);
    virtual int pickTask(DependencyGraph::Edge &edge,
                         DependencyGraph::Edge &negationEdge,
                         Message &message,
                         int distance);
};
}
#endif // BASICSEARCHSTRATEGY_H
