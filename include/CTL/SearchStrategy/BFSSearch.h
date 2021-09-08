/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:50 PM
 */

#ifndef BFSSEARCH_H
#define BFSSEARCH_H
#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"
#include <queue>

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class BFSSearch : public SearchStrategy {

  protected:
    [[nodiscard]] auto waiting_size() const -> size_t override { return _waiting.size(); };
    void push_to_waiting(DependencyGraph::Edge *edge) override { _waiting.push(edge); };
    auto pop_from_waiting() -> DependencyGraph::Edge * override {
        auto e = _waiting.front();
        _waiting.pop();
        return e;
    };
    std::queue<DependencyGraph::Edge *> _waiting;
};

} // namespace CTL::SearchStrategy
#endif /* BFSSEARCH_H */
