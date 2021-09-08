/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:51 PM
 */

#include "CTL/SearchStrategy/HeuristicSearch.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"

namespace CTL::SearchStrategy {

auto HeuristicSearch::waiting_size() const -> size_t { return _waiting.size(); }

void HeuristicSearch::push_to_waiting(DependencyGraph::Edge *edge) { _waiting.push_back(edge); }

auto HeuristicSearch::pop_from_waiting() -> DependencyGraph::Edge * {
    auto it = std::max_element(_waiting.begin(), _waiting.end(), [](auto a, auto b) {
        return false; // a->targets.size() < b->targets.size();
    });
    auto edge = *it;
    _waiting.erase(it);
    return edge;
}
} // namespace CTL::SearchStrategy
