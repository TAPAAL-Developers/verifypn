#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include <cstdio>
#include <stack>
#include <queue>

#include "../DependencyGraph/Edge.h"

namespace SearchStrategy {

class AbstractSearchStrategy
{
public:
    virtual bool done() =0;
    virtual bool push(DependencyGraph::Edge *e) =0;
    virtual DependencyGraph::Edge *pickTask() =0;
    //virtual std::vector<Configuration*> pickTargets(Edge *e) =0;
};
/*
class DistributedSearchStrategy : public AbstractSearchStrategy
{
    virtual bool shouldPickMessage() =0;
    virtual Message *pickMessage() =0;
    virtual void pushMessage(int receiver, Message &m) =0;
};
*/
/*
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
    void pushDependency(Edge *e) { S.push(e); }
    bool empty(){ return S.empty(); }
    size_t size() { return S.size(); }   
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
    void pushDependency(Edge *e) { S.push(e); }
    bool empty(){ return S.empty(); }
    size_t size() { return S.size(); }
};*/

}
#endif // SEARCHSTRATEGY_H
