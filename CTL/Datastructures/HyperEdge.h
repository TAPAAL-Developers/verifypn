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

    inline void add_target(BaseConfiguration *t_target);

    BaseConfiguration *source;
    std::list<BaseConfiguration*> targets;

    bool deleted = false;
    bool processed = false;

    typedef std::list<ctl::BaseConfiguration*>::iterator iterator;
};
}

#endif // HYPEREDGE_H
