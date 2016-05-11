#ifndef NEGATIONWAITINGLIST_H
#define NEGATIONWAITINGLIST_H

#include "iWaitingList.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"
#include <deque>

namespace SearchStrategy {

class NegationWaitingList : public iNegationList
{
    using Edge = DependencyGraph::Edge;
    std::deque<Edge*> safe_edges;
    std::vector<std::vector<Edge*>> unsafe_edges;
    unsigned int _maxDistance = 0;
    std::size_t _size = 0;

    // iWaitingList interface
public:
    virtual bool empty() const override;
    virtual std::size_t size() const override;
    virtual bool pop(DependencyGraph::Edge *&t) override;
    virtual void push(DependencyGraph::Edge *&t) override;

    // iNegationList interface
public:
    virtual unsigned int maxDistance() const override;
    virtual void releaseNegationEdges(unsigned int dist) override;
};
}
#endif // NEGATIONWAITINGLIST_H
