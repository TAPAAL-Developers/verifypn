#ifndef PARTITIONFUNCTION_H
#define PARTITIONFUNCTION_H

#include "../DependencyGraph/Configuration.h"

namespace Algorithm {

class PartitionFunction
{
public:
    virtual int ownerId(DependencyGraph::Configuration *v) =0;
};

}
#endif // PARTITIONFUNCTION_H
