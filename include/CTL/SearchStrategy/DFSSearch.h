#ifndef DFSSEARCH_H
#define DFSSEARCH_H

#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"
#include <stack>

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class DFSSearch : public SearchStrategy {

  protected:
    size_t waiting_size() const { return _waiting.size(); };
    void push_to_waiting(DependencyGraph::Edge *edge) { _waiting.push(edge); };
    DependencyGraph::Edge *pop_from_waiting() {
        auto e = _waiting.top();
        _waiting.pop();
        return e;
    };
    std::stack<DependencyGraph::Edge *> _waiting;
};

} // namespace CTL::SearchStrategy
#endif // DFSSEARCH_H
