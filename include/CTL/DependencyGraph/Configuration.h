#ifndef CONFIGURATION_H
#define CONFIGURATION_H

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
    std::forward_list<Edge*> _dependency_set;
    uint32_t _nsuccs = 0;
private:
    uint32_t _distance = 0;
    void set_distance(uint32_t value) { _distance = value; }
public:
    int8_t _assignment = UNKNOWN;
    Configuration() {}
    uint32_t get_distance() const { return _distance; }
    bool is_done() const { return _assignment == ONE || _assignment == CZERO; }
    void add_dependency(Edge* e);
    void set_owner(uint32_t) { }
    uint32_t get_owner() { return 0; }

};


}
#endif // CONFIGURATION_H
