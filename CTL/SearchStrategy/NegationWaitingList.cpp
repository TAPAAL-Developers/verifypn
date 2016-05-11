#include "NegationWaitingList.h"
#include <assert.h>
#include <iostream>

bool SearchStrategy::NegationWaitingList::empty() const
{
    return size() == 0 ? true : false;
}

std::size_t SearchStrategy::NegationWaitingList::size() const
{
    return _size;
}

bool SearchStrategy::NegationWaitingList::pop(DependencyGraph::Edge *&t)
{
    if(!safe_edges.empty()){
        _size--;
        t = safe_edges.front();
        safe_edges.pop_front();

        return true;
    }
    else
        return false;
}

void SearchStrategy::NegationWaitingList::push(DependencyGraph::Edge *&t)
{
    _size++;
    auto distance = t->source->getDistance();

    //Enlarge container if necessary
    if(unsafe_edges.size() < distance){
        unsafe_edges.resize(distance);
    }

    unsafe_edges[distance].push_back(t);
    _maxDistance = std::max(distance, _maxDistance);
}

unsigned int SearchStrategy::NegationWaitingList::maxDistance() const
{
    return _maxDistance;
}

void SearchStrategy::NegationWaitingList::releaseNegationEdges(unsigned int dist)
{
//    std::cout << "Dist: " << dist << std::endl;

    for(int i = unsafe_edges.size(); i >= dist; i--)
    {
        auto &edges = unsafe_edges[i];
        auto begin = edges.begin();
        auto end = edges.end();

        safe_edges.insert(safe_edges.end(), begin, end);
        edges.clear();
    }

}
