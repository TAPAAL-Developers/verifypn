#ifndef IWAITINGLIST_H
#define IWAITINGLIST_H

#include "../DependencyGraph/Edge.h"

namespace SearchStrategy {

struct Message;

template<class T>
class iWaitingList {
public:
    virtual bool empty() const =0;
    virtual std::size_t size() const =0;
    virtual bool pop(T& t) =0;
    virtual void push(T& t) =0;
};

class iNegationList : public iWaitingList<DependencyGraph::Edge*> {
public:
    virtual unsigned int maxDistance() const =0;
    virtual void releaseNegationEdges(unsigned int dist) =0;
};

class iEdgeList : public iWaitingList<DependencyGraph::Edge*>{
};

class iMassageList : public iWaitingList<SearchStrategy::Message>{
};

}
#endif // IWAITINGLIST_H
