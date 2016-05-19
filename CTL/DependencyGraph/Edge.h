#ifndef EDGE_H
#define EDGE_H

#include <cstdio>
#include <vector>
#include <string>

namespace DependencyGraph {

class Configuration;

class Edge {
public:
    typedef std::vector<Configuration*> container_type;
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    Configuration* source;
    Configuration* requested = nullptr;
    container_type targets;

    bool processed = false;
    bool is_deleted = false;
    bool is_negated = false;

    virtual std::string toString();
};
}
#endif // EDGE_H
