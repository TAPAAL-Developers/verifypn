#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "logging.h"

#include "Edge.h"

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <forward_list>

namespace DependencyGraph {

class Edge;

class Configuration
{
public:
    std::forward_list<Edge*> dependency_set;   
    uint32_t nsuccs = 0;
private:
    uint32_t distance = 0;
    void setDistance(uint32_t value) { distance = value; }
public:
    int8_t assignment = UNKNOWN;
#if DEBUG_DETAILED
    size_t id;
#endif
    Configuration() {}
    uint32_t getDistance() const { return distance; }
    bool isDone() const { return assignment == ONE || assignment == CZERO; }
    void addDependency(Edge* e);
    void setOwner(uint32_t) { }
    uint32_t getOwner() { return 0; }
    
};

    std::string to_string(Assignment a);

}
#endif // CONFIGURATION_H
