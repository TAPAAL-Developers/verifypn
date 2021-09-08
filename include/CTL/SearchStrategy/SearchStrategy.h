#ifndef ISEARCHSTRATEGY_H
#define ISEARCHSTRATEGY_H

#include "CTL/DependencyGraph/Edge.h"
#include <sstream>

namespace CTL::SearchStrategy {

class SearchStrategy {
  public:
    virtual ~SearchStrategy(){};
    bool empty() const;
    void push_edge(DependencyGraph::Edge *edge);
    void push_dependency(DependencyGraph::Edge *edge);
    void push_negation(DependencyGraph::Edge *edge);
    DependencyGraph::Edge *pop_edge(bool saturate = false);
    size_t size() const;
    uint32_t max_distance() const;
    void release_negation_edges(uint32_t);
    bool trivial_negation();
    virtual void flush(){};

  protected:
    virtual size_t waiting_size() const = 0;
    virtual void push_to_waiting(DependencyGraph::Edge *edge) = 0;
    virtual DependencyGraph::Edge *pop_from_waiting() = 0;

    std::vector<DependencyGraph::Edge *> _negation;
    std::vector<DependencyGraph::Edge *> _dependencies;
};

} // namespace CTL::SearchStrategy
#endif // SEARCHSTRATEGY_H
