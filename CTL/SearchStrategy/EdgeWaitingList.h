#ifndef EDGEWAITINGLIST_H
#define EDGEWAITINGLIST_H

#include "iWaitingList.h"
#include <queue>

namespace SearchStrategy {

template<class Container>
class EdgeWaitingList : public iEdgeList
{
    using Edge = DependencyGraph::Edge;
    Container edges;
public:
    EdgeWaitingList(Container C = Container()) : edges(C) {}

    // iWaitingList interface
public:
    virtual bool empty() const { return edges.empty();}
    virtual std::size_t size() const { return edges.size(); }
    virtual bool pop(Edge *&t) {
        if(!edges.empty()){
            t = edges.top();
            edges.pop();
            return true;
        }
        else
            return false;
    }

    virtual void push(Edge *&t) { edges.push(t); }
};

template<>
class EdgeWaitingList<std::queue<DependencyGraph::Edge*>> : public iEdgeList
{
    using Edge = DependencyGraph::Edge;
    using Container = std::queue<Edge*>;
    Container edges;
public:
    EdgeWaitingList(Container C = Container()) : edges(C) {}

    // iWaitingList interface
public:
    virtual bool empty() const { return edges.empty();}
    virtual std::size_t size() const { return edges.size(); }
    virtual bool pop(Edge *&t) {
        if(!edges.empty()){
            t = edges.front();
            edges.pop();
            return true;
        }
        else
            return false;
    }

    virtual void push(Edge *&t) { edges.push(t); }
};

}
#endif // EDGEWAITINGLIST_H
