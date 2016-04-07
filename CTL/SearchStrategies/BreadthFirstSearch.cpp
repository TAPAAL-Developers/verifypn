#include "BreadthFirstSearch.h"

ctl::BreadthFirstSearch::BreadthFirstSearch()
{

}

void ctl::BreadthFirstSearch::push(ctl::Edge *e)
{
    W.push_back(e);
}

void ctl::BreadthFirstSearch::push_dependency(ctl::Edge *e)
{
    W.push_back(e);
}

ctl::Edge *ctl::BreadthFirstSearch::pop()
{
    Edge *e = W.front();
    W.pop_front();
    return e;
}

bool ctl::BreadthFirstSearch::remove(ctl::Edge *e)
{
    iterator current = W.begin();
    iterator end = W.end();

    while(current != end){
        if((*current) == e){
            W.erase(current);
            return true;
        }
    }

    return false;
}

size_t ctl::BreadthFirstSearch::size() {return W.size();}

bool ctl::BreadthFirstSearch::empty() {return W.empty();}

void ctl::BreadthFirstSearch::clear(){ W.clear();}
