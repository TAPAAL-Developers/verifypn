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
    if(_size > 0 && dist <= _maxDistance){
        int i = _maxDistance;

        std::cout << "i: " << i << " dist: " << dist << std::endl;
        for(; i >= dist; i--){
            std::cout << "Releasing edges with degree: " << i << std::endl;
            auto &edges = unsafe_edges[i];

            if(!edges.empty()){
                std::cout << "inserting edges" << std::endl;
                safe_edges.insert(safe_edges.end(), edges.begin(), edges.end());
                edges.clear();
            }
        }
    }

    std::cout << "Done releasing edges" << std::endl;
}
