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
    void flush();

  protected:
    size_t waiting_size() const;
    void push_to_waiting(DependencyGraph::Edge *edge);
    DependencyGraph::Edge *pop_from_waiting();
    std::vector<DependencyGraph::Edge *> W;
    size_t _last_parent = 0;
};

} // namespace CTL::SearchStrategy

#endif /* RDFSSEARCH_H */
