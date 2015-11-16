#ifndef EDGE_H
#define EDGE_H

#include <list>

namespace ctl {

//To avoid circular dependency
class Configuration;

class Edge
{
public:
    Edge(){};

    Configuration* Source;
    std::list<Configuration*> Targets;

    bool operator==(const Edge & rhs) const;
};

}
#endif // EDGE_H
