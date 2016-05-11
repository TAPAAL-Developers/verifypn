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

void SearchStrategy::NegationWaitingList::_push(SearchStrategy::NegationWaitingList::Edge *e)
{
    auto distance = e->source->getDistance();

    //Enlarge container if necessary
    if(unsafe_edges.size() < distance + 1){
        unsafe_edges.reserve(distance + 1); //Enlarge vector
        unsafe_edges.resize(distance + 1);  //Fill vector with empty elements
    }

    unsafe_edges[distance].push_back(e);
}

bool SearchStrategy::NegationWaitingList::empty() const
{
    return _size == 0 ? true : false;
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

void SearchStrategy::NegationWaitingList::push(DependencyGraph::Edge *&e)
{
    _size++;
    _push(e);
    _maxDistance = std::max(e->source->getDistance(), _maxDistance); //Update max distance
}

unsigned int SearchStrategy::NegationWaitingList::maxDistance() const
{
    return _maxDistance;
}

void SearchStrategy::NegationWaitingList::releaseNegationEdges(unsigned int dist)
{
//    std::cout << "Dist: " << dist << " maxDist " << _maxDistance << " size: " << _size << std::endl;

    //We have edges that can be released
    assert(dist == _maxDistance);
    if(_size > 0 && dist == _maxDistance){

        if(!unsafe_edges[dist].empty()){

            for(Edge *e : unsafe_edges[dist])
            {
                if(e->source->isDone()){
                    _size--;
//                    std::cout << "Was done" << std::endl;
                    e->source->printConfiguration();
                }
                //If this is true, that means a configuration changed dist
                //that should not be possible for petrinets
                else if(e->source->getDistance() != _maxDistance){
                    assert(false && "This should not happen");
                }
                else {
                    safe_edges.push_back(e);
                }
            }

            unsafe_edges[dist].clear();
        }
        else {
//            std::cout << "Couldn't release any edges" << std::endl;
        }

        _maxDistance = computeMaxDistance();
    }

//    int index = 0;
//    for(std::vector<Edge*> v : unsafe_edges){
//        std::cout << "Vector: " << index << " size " << v.size() << std::endl;
//        index++;
//    }

//    std::cout << "New max distance: " << _maxDistance
//              << " Size of safe: " << safe_edges.size()
//              << " Size of N: " << _size
//              << std::endl;
}
