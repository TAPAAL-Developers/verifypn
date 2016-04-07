#ifndef BREADTHFIRSTSEARCH_H
#define BREADTHFIRSTSEARCH_H

#include <deque>
#include "AbstractSearchStrategy.h"

namespace ctl{

class BreadthFirstSearch : public ctl::AbstractSearchStrategy
{
public:
    BreadthFirstSearch();

    // AbstractSearchStrategy interface
    void push(Edge *e);
    void push_dependency(Edge *e);
    Edge *pop();
    bool remove(Edge *e);
    size_t size();
    bool empty();
    void clear();

private:
    std::deque<Edge*> W; //Waiting list
    typedef std::deque<Edge*>::iterator iterator;
};
}
#endif // BREADTHFIRSTSEARCH_H
