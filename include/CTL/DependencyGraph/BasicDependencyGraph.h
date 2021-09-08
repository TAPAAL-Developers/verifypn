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
    virtual std::vector<Edge *> successors(Configuration *c) = 0;
    virtual Configuration *initial_configuration() = 0;
    virtual void release(Edge *e) = 0;
};

} // namespace DependencyGraph
#endif // ABSTRACTDEPENDENCYGRAPH_H
