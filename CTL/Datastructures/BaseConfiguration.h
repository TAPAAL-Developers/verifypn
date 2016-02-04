#ifndef BASECONFIGURATION_H
#define BASECONFIGURATION_H

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
    void config_printer() =0;

    Assignment assignment = UNKNOWN;

    std::list<HyperEdge*> dependencies;
    std::list<HyperEdge*> successors;
    std::list<HyperEdge*> deleted_sucessors;

private:
    typedef std::list<HyperEdge*>::iterator iterator;
};
}
#endif // CONFIGURATION_H
