/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:52 PM
 */

#ifndef RDFSSEARCH_H
#define RDFSSEARCH_H

#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"
#include <deque>

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class RDFSSearch : public SearchStrategy {
  public:
    void flush() override;

  protected:
    [[nodiscard]] auto waiting_size() const -> size_t override;
    void push_to_waiting(DependencyGraph::Edge *edge) override;
    auto pop_from_waiting() -> DependencyGraph::Edge * override;
    std::vector<DependencyGraph::Edge *> _waiting;
    size_t _last_parent = 0;
};

} // namespace CTL::SearchStrategy

#endif /* RDFSSEARCH_H */
