#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include <cstdio>
#include <stack>
#include <queue>

namespace DependencyGraph {

class Edge;

//Search strategy interface
class AbstractSearchStrategy
{
public:
    virtual void pop() =0;
    virtual Edge *top() =0;
    virtual void push(Edge *e) =0;
    virtual void push_dependency(Edge *e) =0;
    //virtual void remove(Edge *e) =0;
    virtual bool empty() =0;
    virtual std::size_t size() =0;
    virtual AbstractSearchStrategy *clone() =0;
};

template<class strategy_type = std::stack<Edge*>>
class SearchStrategy : AbstractSearchStrategy {
protected:
    strategy_type S;

    // AbstractSearchStrategy interface
public:

    SearchStrategy(strategy_type S = strategy_type()) : S(S){}

    void pop() { S.pop(); }
    Edge *top() { return S.top(); }
    void push(Edge *e) {S.push(e); }
    void push_dependency(Edge *e) { S.push(e); }
    //void remove(Edge *e){}
    bool empty(){ return S.empty(); }
    size_t size() { return S.size(); }
    AbstractSearchStrategy *clone() { return new SearchStrategy<strategy_type>(*this); }
};

template<>
class SearchStrategy<std::queue<Edge*>> : public AbstractSearchStrategy {
     using queue = typename std::queue<Edge*>;
    queue S;
    // AbstractSearchStrategy interface

public:

    SearchStrategy<queue>(queue S = queue()) : S(S){}

    void pop() { S.pop(); }
    Edge *top() { return S.front(); }
    void push(Edge *e) {S.push(e); }
    void push_dependency(Edge *e) { S.push(e); }
    //void remove(Edge *e){}
    bool empty(){ return S.empty(); }
    size_t size() { return S.size(); }
    AbstractSearchStrategy *clone() { return new SearchStrategy<queue>(*this); }
};

}
#endif // SEARCHSTRATEGY_H
