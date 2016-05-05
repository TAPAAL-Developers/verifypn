#ifndef ABSTRACTDEPENDENCYGRAPH_H
#define ABSTRACTDEPENDENCYGRAPH_H

#include <cstddef>
#include <vector>

namespace DependencyGraph {

class Configuration;

class BasicDependencyGraph {
public:
    virtual void successors(Configuration *c) =0;
    virtual Configuration *initialConfiguration() =0;
    virtual void cleanUp() =0;
};
/*
class SerializableDependencyGraph : public virtual BasicDependencyGraph {
public:
    virtual std::vector<int> *serialize(Configuration &c) =0;
    virtual Configuration *deserialize(std::vector<int> &vec) =0;
};
*/
}
#endif // ABSTRACTDEPENDENCYGRAPH_H
