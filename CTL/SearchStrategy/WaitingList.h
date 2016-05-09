#ifndef WAITINGLIST_H
#define WAITINGLIST_H

#include <stack>
#include <queue>

namespace SearchStrategy {

struct Message;

template<class T, class Container>
class WaitingList {
    Container c;

public:
    WaitingList(Container S = Container()) : c(S){}
    void pop() { c.pop(); }
    T top() { return c.top(); }
    void push(T e) {c.push(e); }
    void pushDependency(T e) { c.push(e); }
    bool empty(){ return c.empty(); }
    std::size_t size() { return c.size(); }
};

template<class T>
class WaitingList<T, std::queue<T>> {
    using queue = typename std::queue<T>;
    queue c;
public:
    WaitingList<T, queue>(queue S = queue()) : c(S){}

    void pop() { c.pop(); }
    T top() { return c.front(); }
    void push(T e) {c.push(e); }
    void pushDependency(T e) { c.push(e); }
    bool empty(){ return c.empty(); }
    std::size_t size() { return c.size(); }
};

template<class edge_container = std::stack<DependencyGraph::Edge*>>
class EdgeWaitingList : public WaitingList<DependencyGraph::Edge*, edge_container> {
};

class MessageWaitingList : public WaitingList<Message, std::queue<Message>> {
};
}
#endif // WAITINGLIST_H
