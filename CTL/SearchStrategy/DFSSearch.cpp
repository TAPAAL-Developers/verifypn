
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

void SearchStrategy::DFSSearch::clear()
{
    while(!W.empty())
        W.pop();
}

SearchStrategy::TaskType SearchStrategy::DFSSearch::pickTask(DependencyGraph::Edge*& edge)
{
    if (W.empty()) return EMPTY;
    edge = W.top();
    W.pop();
    return EDGE;
}
