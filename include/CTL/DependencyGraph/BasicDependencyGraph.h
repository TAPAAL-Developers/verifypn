#ifndef ABSTRACTDEPENDENCYGRAPH_H
#define ABSTRACTDEPENDENCYGRAPH_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace DependencyGraph {

class Configuration;
class Edge;

class BasicDependencyGraph {

  public:
    virtual auto successors(Configuration *c) -> std::vector<Edge *> = 0;
    virtual auto initial_configuration() -> Configuration * = 0;
    virtual void release(Edge *e) = 0;
};

} // namespace DependencyGraph
#endif // ABSTRACTDEPENDENCYGRAPH_H
