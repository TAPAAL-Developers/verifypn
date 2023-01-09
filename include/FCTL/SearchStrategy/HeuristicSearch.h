/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:51 PM
 */

#ifndef FEATURED_HEURISTICSEARCH_H
#define FEATURED_HEURISTICSEARCH_H

#include <vector>
#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "SearchStrategy.h"

namespace Featured {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class HeuristicSearch : public SearchStrategy {

protected:
    size_t Wsize() const;
    void pushToW(DependencyGraph::Edge* edge);
    DependencyGraph::Edge* popFromW();
    std::vector<DependencyGraph::Edge*> W;
};

}   // end SearchStrategy

#endif /* HEURISTICSEARCH_H */

