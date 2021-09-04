/* 
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on March 7, 2018, 1:50 PM
 */

#ifndef BFSSEARCH_H
#define BFSSEARCH_H
#include <queue>
#include "CTL/DependencyGraph/Edge.h"
#include "SearchStrategy.h"

namespace CTL::SearchStrategy {

// A custom search strategy that should ensure as little overhead as possible
// while running sequential computation.

class BFSSearch : public SearchStrategy {

protected:
    size_t waiting_size() const { return _waiting.size(); };
    void push_to_waiting(DependencyGraph::Edge* edge) { _waiting.push(edge); };
    DependencyGraph::Edge* pop_from_waiting() 
    {
        auto e = _waiting.front();
        _waiting.pop();
        return e;
    };    
    std::queue<DependencyGraph::Edge*> _waiting;
};

}   // end SearchStrategy
#endif /* BFSSEARCH_H */

