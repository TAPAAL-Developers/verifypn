#include "NegationWaitingList.h"
#include <assert.h>
#include <iostream>

unsigned int SearchStrategy::NegationWaitingList::computeMaxDistance() const
{
    auto maxDist = unsafe_edges.size() - 1;

    while(maxDist != 0){
        if(!unsafe_edges[maxDist].empty()){
            break;
        }

        maxDist--;
    }

    return maxDist;
}

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
    if(unsafe_edges.size() < distance + 1){
        unsafe_edges.reserve(distance + 1); //Enlarge vector
        unsafe_edges.resize(distance + 1);  //Fill vector with empty elements
    }

    unsafe_edges[distance].push_back(t);
    _maxDistance = std::max(distance, _maxDistance); //Update max distance
}

unsigned int SearchStrategy::NegationWaitingList::maxDistance() const
{
    return _maxDistance;
}

void SearchStrategy::NegationWaitingList::releaseNegationEdges(unsigned int dist)
{
    std::cout << "Dist: " << dist << " size: " << unsafe_edges.size() << std::endl;

    //We have edges that can be released
    if(!empty() && dist <= _maxDistance){
        int i = _maxDistance;

        std::cout << "i: " << i << " dist: " << dist << std::endl;
        for(; i >= (int)dist; --i){
            std::cout << "Releasing edges with degree: " << i << std::endl;
            auto &edges = unsafe_edges[i];

            if(!edges.empty()){
                std::cout << "inserting edges" << std::endl;
                safe_edges.insert(safe_edges.end(), edges.begin(), edges.end());
                edges.clear();
            }
        }

        _maxDistance = computeMaxDistance();

        std::cout << "New max distance: " << _maxDistance << std::endl;
    }

    std::cout << "Done releasing edges" << std::endl;
}
