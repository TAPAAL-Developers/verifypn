#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "Edge.h"

#include <cstdio>
#include <forward_list>
#include <iostream>
#include <string>
#include <vector>

namespace DependencyGraph {

class Edge;

class Configuration {
  public:
    std::forward_list<Edge *> _dependency_set;
    uint32_t _nsuccs = 0;

  private:
    uint32_t _distance = 0;
    void set_distance(uint32_t value) { _distance = value; }

  public:
    int8_t _assignment = UNKNOWN;
    Configuration() = default;
    [[nodiscard]] auto get_distance() const -> uint32_t { return _distance; }
    [[nodiscard]] auto is_done() const -> bool { return _assignment == ONE || _assignment == CZERO; }
    void add_dependency(Edge *e);
};

} // namespace DependencyGraph
#endif // CONFIGURATION_H
