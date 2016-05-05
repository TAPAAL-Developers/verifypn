#ifndef DEPTHFIRSTSEARCH_H
#define DEPTHFIRSTSEARCH_H

#include <deque>
#include "AbstractSearchStrategy.h"

namespace ctl{

class DepthFirstSearch : public ctl::AbstractSearchStrategy
{
public:
    DepthFirstSearch();

    // AbstractSearchStrategy interface
    void push(Edge *e);
    void push_dependency(Edge *e);
    Edge *pop();
    bool remove(Edge *e);
    size_t size() {return W.size();}
    bool empty() {return W.empty();}
    void clear(){ W.clear();}

private:
    std::deque<Edge*> W; //Waiting list
    typedef std::deque<Edge*>::iterator iterator;
};
}
#endif // DEPTHFIRSTSEARCH_H
