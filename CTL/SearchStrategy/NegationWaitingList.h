#ifndef NEGATIONWAITINGLIST_H
#define NEGATIONWAITINGLIST_H

#include "iWaitingList.h"

namespace SearchStrategy {

class NegationWaitingList : public iNegationList
{

public:
    // iWaitingList interface
private:
    virtual bool empty();
    virtual std::size_t size();
    virtual bool pop(DependencyGraph::Edge *&t);
    virtual void push(DependencyGraph::Edge *&t);

    // iNegationList interface
public:
    virtual void releaseNegationEdges(int dist);
};
}
#endif // NEGATIONWAITINGLIST_H
