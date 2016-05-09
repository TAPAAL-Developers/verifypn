
#include <iostream>
#include "assert.h"
#include "DFSSearch.h"

bool SearchStrategy::DFSSearch::empty()
{
    return W.empty();
}

void SearchStrategy::DFSSearch::pushEdge(DependencyGraph::Edge *edge)
{
    W.push(edge);
}

void SearchStrategy::DFSSearch::pushMessage(SearchStrategy::Message &message)
{
    std::cerr << "Basic DFS search strategy can't handle communication" << std::endl;
    assert(false);
}

SearchStrategy::AbstractSearchStrategy::TaskType SearchStrategy::DFSSearch::pickTask(DependencyGraph::Edge*& edge, SearchStrategy::Message*& message)
{
    if (W.empty()) return EMPTY;
    edge = W.top();
    W.pop();
    return EDGE;
}
