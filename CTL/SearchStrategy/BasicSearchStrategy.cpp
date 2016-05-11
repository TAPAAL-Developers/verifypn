#include "BasicSearchStrategy.h"
#include <assert.h>

bool SearchStrategy::BasicSearchStrategy::empty() const
{
    return W->empty() && N->empty();
}

void SearchStrategy::BasicSearchStrategy::pushEdge(SearchStrategy::BasicSearchStrategy::Edge *edge)
{
    if(edge->is_negated){
        N->push(edge);
    }
    else
    {
        W->push(edge);
    }
}

SearchStrategy::TaskType SearchStrategy::BasicSearchStrategy::pickTask(SearchStrategy::BasicSearchStrategy::Edge *&edge)
{
    if(W->empty()){
        N->releaseNegationEdges(N->maxDistance());
    }

    if(N->pop(edge))
        return TaskType::EDGE;
    else if(W->pop(edge))
        return TaskType::EDGE;

    assert(W->empty() && N->empty());
    return TaskType::EMPTY;
}

//SearchStrategy::iEdgeList SearchStrategy::BasicSearchStrategy::EdgeList() const
//{
//    return W;
//}

//void SearchStrategy::BasicSearchStrategy::EdgeList(const SearchStrategy::iEdgeList &value)
//{
//    W = value;
//}

//SearchStrategy::iNegationList SearchStrategy::BasicSearchStrategy::NegationList() const
//{
//    return N;
//}

//void SearchStrategy::BasicSearchStrategy::NegationList(const SearchStrategy::iNegationList &value)
//{
//    N = value;
//}
