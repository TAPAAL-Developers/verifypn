#include "DepthFirstSearch.h"

ctl::DepthFirstSearch::DepthFirstSearch()
{
}

void ctl::DepthFirstSearch::push(ctl::Edge *e)
{
    W.push_back(e);
}

void ctl::DepthFirstSearch::push_dependency(ctl::Edge *e)
{
    W.push_back(e);
}

ctl::Edge *ctl::DepthFirstSearch::pop()
{
    Edge *e = W.back();
    W.pop_back();
    return e;
}

bool ctl::DepthFirstSearch::remove(ctl::Edge *e)
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
