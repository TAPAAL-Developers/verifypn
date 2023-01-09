#ifndef FEATURED_DFSSEARCH_H
#define FEATURED_DFSSEARCH_H

#include <stack>
#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "SearchStrategy.h"

namespace Featured {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class DFSSearch : public SearchStrategy {

protected:
    size_t Wsize() const { return W.size(); };
    void pushToW(DependencyGraph::Edge* edge) { W.push(edge); };
    DependencyGraph::Edge* popFromW() 
    {
        auto e = W.top();
        W.pop();
        return e;
    };
    std::stack<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif // DFSSEARCH_H
