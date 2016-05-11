#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include "iSearchStrategy.h"

#include <stack>

namespace SearchStrategy {

class DFSSearch : public iSequantialSearchStrategy, public iClearable
{
public:
    DFSSearch() {}
    virtual bool empty() const override;
    virtual void pushEdge(DependencyGraph::Edge *edge) override;
    virtual void clear() override;
    virtual TaskType pickTask(DependencyGraph::Edge*& edge) override;

protected:

    std::stack<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif // DFSSEARCH_H
