
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

void SearchStrategy::DFSSearch::pushNegationEdge(DependencyGraph::Edge *edge)
{
    W.push(edge);
}

void SearchStrategy::DFSSearch::pushMessage(SearchStrategy::Message &message)
{
    std::cerr << "Basic DFS search strategy can't handle communication" << std::endl;
    assert(false);
}

int SearchStrategy::DFSSearch::pickTask(DependencyGraph::Edge*& edge, DependencyGraph::Edge*& negationEdge, SearchStrategy::Message*& message, int distance)
{
    if (W.empty()) return -1;
    DependencyGraph::Edge *e = W.top();
    W.pop();
    if (e->source->is_negated) {
        negationEdge = e;
        return 1;
    } else {
        edge = e;
        return 0;
    }
}
