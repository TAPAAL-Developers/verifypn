#ifndef EDGE_H1
#define EDGE_H1
//TODO: Fix Guard

#include <cstdio>
#include <vector>

namespace DependencyGraph {

class Configuration;

class Edge {
    typedef std::vector<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    Configuration* source;
    container targets;

    bool processed = false;
    bool is_deleted = false;
    bool is_negated = false;
};
}
#endif // EDGE_H
