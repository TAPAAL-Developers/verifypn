/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 * 
 * Created on March 7, 2018, 1:51 PM
 */

#include "FCTL/SearchStrategy/HeuristicSearch.h"
#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "FCTL/DependencyGraph/FConfiguration.h"

namespace Featured {

    size_t HeuristicSearch::Wsize() const {
        return W.size();
    }

    void HeuristicSearch::pushToW(DependencyGraph::Edge* edge) {
        W.push_back(edge);
    }

    DependencyGraph::Edge* HeuristicSearch::popFromW() {
        auto it = std::max_element(W.begin(), W.end(), [](auto a, auto b){
            return false; // a->targets.size() < b->targets.size();
        });
        auto edge = *it;
        W.erase(it);
        return edge;
    }  
}
