#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include <stack>
#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class DFSSearch : public SearchStrategy {

protected:
    size_t waiting_size() const { return W.size(); };
    void push_to_waiting(DependencyGraph::Edge* edge) { W.push(edge); };
    DependencyGraph::Edge* pop_from_waiting() 
    {
        auto e = W.top();
        W.pop();
        return e;
    };
    std::stack<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy
#endif // DFSSEARCH_H
