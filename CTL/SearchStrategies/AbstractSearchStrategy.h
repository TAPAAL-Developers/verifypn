#ifndef ABSTRACTSEARCHSTRATEGY_H
#define ABSTRACTSEARCHSTRATEGY_H

#include <cstddef>

namespace ctl {

class Edge; //Forward declaration of edge

class AbstractSearchStrategy {
public:
    virtual void push(Edge *e) =0;
    virtual void push_dependency(Edge *e) =0;
    virtual Edge *pop() =0;
    virtual bool remove(Edge *e) =0;
    virtual size_t size() =0;
    virtual bool empty() =0;
    virtual void clear()=0;
};

}

#endif // ABSTRACTSEARCHSTRATEGY_H
