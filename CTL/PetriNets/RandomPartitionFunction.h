#ifndef RANDOMPARTITIONFUNCTION_H
#define RANDOMPARTITIONFUNCTION_H


#include "../Algorithm/PartitionFunction.h"
#include "Marking.h"
#include "PetriConfig.h"
#include <stdint.h>

namespace PetriNets {

class RandomPartitionFunction : public Algorithm::PartitionFunction
{
public:
    RandomPartitionFunction(int worker_count) : worker_count(worker_count) {}

    std::hash<Marking*> hasher;

    virtual int ownerId(DependencyGraph::Configuration *c) override {        
        PetriConfig *v = static_cast<PetriConfig*>(c);
        srand(hasher(v->marking));
        return (rand()) % worker_count;
    }

protected:
    int worker_count;
};

}

#endif // RANDOMPARTITIONFUNCTION_H
