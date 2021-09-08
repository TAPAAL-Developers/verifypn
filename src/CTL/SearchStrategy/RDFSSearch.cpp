/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:52 PM
 */

#include "CTL/SearchStrategy/RDFSSearch.h"
#include "CTL/DependencyGraph/Configuration.h"

#include <algorithm>
#include <random>

namespace CTL::SearchStrategy {
auto RDFSSearch::waiting_size() const -> size_t { return W.size(); }

auto RDFSSearch::pop_from_waiting() -> DependencyGraph::Edge * {
    auto e = W.back();
    W.pop_back();
    _last_parent = W.size();
    return e;
}

void RDFSSearch::push_to_waiting(DependencyGraph::Edge *edge) {
    _last_parent = std::min(W.size(), _last_parent);
    W.push_back(edge);
}

auto rng = std::default_random_engine{};
void RDFSSearch::flush() {
    _last_parent = std::min(_last_parent, W.size());
    std::shuffle(W.begin() + _last_parent, W.end(), rng);
    _last_parent = W.size();
}

} // namespace CTL::SearchStrategy
