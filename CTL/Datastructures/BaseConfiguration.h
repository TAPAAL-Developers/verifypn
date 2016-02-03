#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <list>
#include "HyperEdge.h"

namespace ctl {

enum Assignment { ZERO, CZERO, UNKNOWN, ONE };

class HyperEdge;

class BaseConfiguration
{
public:
    BaseConfiguration(){}
    virtual ~BaseConfiguration();

    void remove_successor(HyperEdge *t_successor);

    Assignment assignment = UNKNOWN;
    bool negated = false;

    std::list<HyperEdge*> dependencies;
    std::list<HyperEdge*> successors;
    std::list<HyperEdge*> deleted_sucessors;

private:
    typedef std::list<HyperEdge*>::iterator iterator;
};
}
#endif // CONFIGURATION_H
