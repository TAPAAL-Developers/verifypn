
#include <iostream>
#include "assert.h"
#include "DFSSearch.h"

bool SearchStrategy::DFSSearch::empty() const
{
    return W.empty();
}

void SearchStrategy::DFSSearch::pushEdge(DependencyGraph::Edge *edge)
{
    W.push_back(edge);
}

void SearchStrategy::DFSSearch::clear()
{
    W.clear();
}

SearchStrategy::TaskType SearchStrategy::DFSSearch::pickTask(DependencyGraph::Edge*& edge)
{
//    std::cout << "Size of W: " << W.size() << std::endl;
    if (W.empty())
        return EMPTY;

    edge = W.back();

    if(edge->is_negated){
//        std::cout << "Negation edge - " << std::boolalpha << edge->processed << std::endl;
        if(edge->processed){
            W.pop_back();
        }
    }
    else {
//        std::cout << "Hyper edge - " << std::boolalpha << edge->processed << std::endl;
        W.pop_back();
    }

    return EDGE;
}
