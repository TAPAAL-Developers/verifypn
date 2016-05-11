#ifndef IWAITINGLIST_H
#define IWAITINGLIST_H

#include "../DependencyGraph/Edge.h"
#include "Collection.h"

namespace SearchStrategy {

struct Message;

template<class T>
class iWaitingList {
    virtual bool empty() =0;
    virtual std::size_t size() =0;
    virtual bool pop(T& t) =0;
    virtual void push(T& t) =0;
};

class iNegationList : public iWaitingList<DependencyGraph::Edge*> {
public:
    virtual void releaseNegationEdges(int dist) =0;
};

class iEdgeList : public iWaitingList<DependencyGraph::Edge*>{
};

class iMassageList : public iWaitingList<SearchStrategy::Message>{
};

}
#endif // IWAITINGLIST_H
