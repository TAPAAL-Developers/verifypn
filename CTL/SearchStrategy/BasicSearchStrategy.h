#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "iSearchStrategy.h"
#include "iWaitingList.h"
#include "NegationWaitingList.h"
#include "EdgeWaitingList.h"

#include <stack>

namespace SearchStrategy {

class BasicSearchStrategy : public iSequantialSearchStrategy {
    using Edge = DependencyGraph::Edge;
    using DefaultEdgeList = EdgeWaitingList<std::stack<Edge*>>;
    using DefaultNegationList = NegationWaitingList;

    iEdgeList *W /*= new DefaultEdgeList()*/;
    iNegationList *N /*= new DefaultNegationList()*/;

public:
//    BasicSearchStrategy() {}
    BasicSearchStrategy(iEdgeList *w = new DefaultEdgeList(),
                        iNegationList *n = new DefaultNegationList()) : W(w), N(n) {}

    // iSequantialSearchStrategy interface
public:
    virtual bool empty() const override;
    virtual void pushEdge(Edge *edge) override;
    virtual TaskType pickTask(Edge *&edge) override;

    //Getters and Setters;
//    iEdgeList EdgeList() const;
//    void EdgeList(const iEdgeList &value);
//    iNegationList NegationList() const;
//    void NegationList(const iNegationList &value);
};


}
#endif // BASICSEARCHSTRATEGY_H
