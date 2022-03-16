#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "Edge.h"
#include "PetriEngine/Structures/SuccessorQueue.h"
#include "PetriEngine/Structures/light_deque.h"

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
private:
    uint32_t distance = 0;
    void setDistance(uint32_t value) { distance = value; }
public:
    int8_t assignment = UNKNOWN;
    bool passed = false;
    bool instack = false;
    bool recheck = false;
    uint32_t rank = std::numeric_limits<uint32_t>::max();

#ifndef NDEBUG
    size_t id;
#endif
    SuccessorQueue<DependencyGraph::Edge*> sucs;
    light_deque<DependencyGraph::Edge*> resucs{1};

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


}
#endif // CONFIGURATION_H
