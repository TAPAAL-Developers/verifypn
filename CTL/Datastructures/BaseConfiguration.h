#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <list>

namespace ctl {

class HyperEdge;

class BaseConfiguration
{
public:
    enum Assignment { ZERO, CZERO, UNKNOWN, ONE };

    BaseConfiguration(){}
    virtual ~BaseConfiguration();
    void remove_sucessors(HyperEdge *e) =0;

    Assignment assignment = UNKNOWN;
    std::list<HyperEdge*> dependencies;
    std::list<HyperEdge*> successors;
};
}
#endif // CONFIGURATION_H
