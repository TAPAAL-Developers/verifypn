#include "BasicSearchStrategy.h"
#include <assert.h>
#include <iostream>

bool SearchStrategy::BasicSearchStrategy::empty() const
{
    return W->empty() && N->empty();
}

void SearchStrategy::BasicSearchStrategy::pushEdge(SearchStrategy::BasicSearchStrategy::Edge *edge)
{
    if(edge->is_negated){
        N->push(edge);
    }
    W->push(edge);
}

SearchStrategy::TaskType SearchStrategy::BasicSearchStrategy::pickTask(SearchStrategy::BasicSearchStrategy::Edge *&edge)
{
//    std::cout << "W is empty: " << std::boolalpha << W->empty() << std::endl;

    do {
        if(N->pop(edge))
            return TaskType::EDGE;
        else if(W->pop(edge))
            return TaskType::EDGE;
        else {
//            std::cout << "Max Dist: " << N->maxDistance() << std::endl;
            N->releaseNegationEdges(N->maxDistance());
        }
    } while (!W->empty() || !N->empty());


//    std::cout << "size of N: " << N->size() << std::endl;
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
