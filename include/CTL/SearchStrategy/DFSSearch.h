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
    [[nodiscard]] auto waiting_size() const -> size_t override { return _waiting.size(); };
    void push_to_waiting(DependencyGraph::Edge *edge) override { _waiting.push(edge); };
    auto pop_from_waiting() -> DependencyGraph::Edge * override {
        auto e = _waiting.top();
        _waiting.pop();
        return e;
    };
    std::stack<DependencyGraph::Edge *> _waiting;
};

} // namespace CTL::SearchStrategy
#endif // DFSSEARCH_H
