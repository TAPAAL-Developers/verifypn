#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "Edge.h"

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <forward_list>
#include <limits>

namespace DependencyGraph {

class Edge;

class Configuration
{
public:
    std::forward_list<Edge*> dependency_set;
#ifdef DG_REFCOUNTING
    std::forward_list<Configuration*> forward_dependency_set;
    uint64_t refc = 0;
#endif
    uint32_t nsuccs = 0;
    std::forward_list<Edge*> successors;
private:
    uint32_t distance = 0;
    void setDistance(uint32_t value) { distance = value; }
public:
    int8_t assignment = UNKNOWN;
    bool passed = false;
    bool on_stack = false;
//#ifndef NDEBUG
    size_t id = std::numeric_limits<size_t>::max();
//#endif
    size_t rank = std::numeric_limits<size_t>::max();
    size_t min_rank = 0;
    Configuration* min_rank_source = nullptr;
    Configuration() {}
    uint32_t getDistance() const { return distance; }
    bool isDone() const { return assignment == ONE || assignment == CZERO; }
    void addDependency(Edge* e);

#ifdef DG_REFCOUNTING
    void remove_dependent(Configuration* c);
#endif
    void setOwner(uint32_t) { }
    uint32_t getOwner() { return 0; }

};
    std::string to_string(Assignment a);

}
#endif // CONFIGURATION_H
