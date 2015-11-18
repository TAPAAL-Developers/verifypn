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
    Edge(Configuration* t_source) : source(t_source) {}

    Configuration* source;
    std::list<Configuration*> targets;

    bool operator==(const Edge & rhs) const;
};

}
#endif // EDGE_H
