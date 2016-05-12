#ifndef HASHPARTITIONFUNCTION_H
#define HASHPARTITIONFUNCTION_H


#include "../Algorithm/PartitionFunction.h"
#include "Marking.h"
#include "PetriConfig.h"

namespace PetriNets {

class HashPartitionFunction : public Algorithm::PartitionFunction
{
public:
    HashPartitionFunction(int worker_count) : worker_count(worker_count) {}

    std::hash<Marking*> hasher;

    virtual int ownerId(DependencyGraph::Configuration *c) override {
        PetriConfig *v = static_cast<PetriConfig*>(c);
        return hasher(v->marking) % worker_count;
    }

protected:
    int worker_count;
};


}

#endif // HASHPARTITIONFUNCTION_H
