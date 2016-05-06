#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include "SearchStrategy.h"

#include <stack>

namespace SearchStrategy {

class DFSSearch : public AbstractSearchStrategy
{
public:
    DFSSearch() {}
    virtual bool empty() override;
    virtual void pushEdge(DependencyGraph::Edge *edge) override;
    virtual void pushNegationEdge(DependencyGraph::Edge *edge) override;
    virtual void pushMessage(Message &message) override;

    virtual int pickTask(DependencyGraph::Edge*& edge,
                         DependencyGraph::Edge*& negationEdge,
                         Message*& message,
                         int distance) override;

protected:

    std::stack<DependencyGraph::Edge*> W;

};

}   // end SearchStrategy

#endif // DFSSEARCH_H
