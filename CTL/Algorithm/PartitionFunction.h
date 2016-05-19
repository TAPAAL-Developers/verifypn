#ifndef PARTITIONFUNCTION_H
#define PARTITIONFUNCTION_H

namespace DependencyGraph {

class Configuration;

}

namespace Algorithm {

class PartitionFunction
{
public:
    virtual int ownerId(DependencyGraph::Configuration *v) =0;
};

}
#endif // PARTITIONFUNCTION_H
