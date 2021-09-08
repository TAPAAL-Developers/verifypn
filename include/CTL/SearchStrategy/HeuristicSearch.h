/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:51 PM
 */

#ifndef HEURISTICSEARCH_H
#define HEURISTICSEARCH_H

#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"
#include <vector>

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class HeuristicSearch : public SearchStrategy {

  protected:
    size_t waiting_size() const;
    void push_to_waiting(DependencyGraph::Edge *edge);
    DependencyGraph::Edge *pop_from_waiting();
    std::vector<DependencyGraph::Edge *> _waiting;
};

} // namespace CTL::SearchStrategy

#endif /* HEURISTICSEARCH_H */
