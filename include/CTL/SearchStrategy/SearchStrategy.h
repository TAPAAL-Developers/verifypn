#ifndef ISEARCHSTRATEGY_H
#define ISEARCHSTRATEGY_H

#include "CTL/DependencyGraph/Edge.h"
#include <sstream>

namespace CTL::SearchStrategy {

class SearchStrategy {
  public:
    virtual ~SearchStrategy()= default;
    [[nodiscard]] auto empty() const -> bool;
    void push_edge(DependencyGraph::Edge *edge);
    void push_dependency(DependencyGraph::Edge *edge);
    void push_negation(DependencyGraph::Edge *edge);
    auto pop_edge(bool saturate = false) -> DependencyGraph::Edge *;
    [[nodiscard]] auto max_distance() const -> uint32_t;
    void release_negation_edges(uint32_t);
    auto trivial_negation() -> bool;
    virtual void flush(){};

  protected:
    [[nodiscard]] virtual auto waiting_size() const -> size_t = 0;
    virtual void push_to_waiting(DependencyGraph::Edge *edge) = 0;
    virtual auto pop_from_waiting() -> DependencyGraph::Edge * = 0;

    std::vector<DependencyGraph::Edge *> _negation;
    std::vector<DependencyGraph::Edge *> _dependencies;
};

} // namespace CTL::SearchStrategy
#endif // SEARCHSTRATEGY_H
