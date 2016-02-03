#ifndef HYPEREDGE_H
#define HYPEREDGE_H

#include <list>

namespace ctl {

class BaseConfiguration;

class HyperEdge
{
public:
    HyperEdge(){}
    HyperEdge(BaseConfiguration *t_source) : source(t_source) {}

    BaseConfiguration *source;
    std::list<BaseConfiguration*> targets;

    bool deleted = false;
    bool processed = false;
};
}

#endif // HYPEREDGE_H
