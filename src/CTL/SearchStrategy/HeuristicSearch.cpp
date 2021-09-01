/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:51 PM
 */

#include "CTL/SearchStrategy/HeuristicSearch.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"

namespace CTL::SearchStrategy {

    size_t HeuristicSearch::waiting_size() const {
        return _waiting.size();
    }

    void HeuristicSearch::push_to_waiting(DependencyGraph::Edge* edge) {
        _waiting.push_back(edge);
    }

    DependencyGraph::Edge* HeuristicSearch::pop_from_waiting() {
        auto it = std::max_element(_waiting.begin(), _waiting.end(), [](auto a, auto b){
            return false; // a->targets.size() < b->targets.size();
        });
        auto edge = *it;
        _waiting.erase(it);
        return edge;
    }
}
