#ifndef DISTRIBUTIVESUMPARTITIONFUNCTION_H
#define DISTRIBUTIVESUMPARTITIONFUNCTION_H


#include "../Algorithm/PartitionFunction.h"
#include "Marking.h"
#include "PetriConfig.h"
#include <stdint.h>

namespace PetriNets {

class DistributiveSumPartitionFunction : public Algorithm::PartitionFunction
{
public:
    DistributiveSumPartitionFunction(int worker_count, int distribution_level)
        : worker_count(worker_count), distribution_level(distribution_level) {}

    virtual int ownerId(DependencyGraph::Configuration *c) override {        
        PetriConfig *v = static_cast<PetriConfig*>(c);
        srand(markingHash(v->marking));

        int sum = 0;
        for (int i = 0; i < v->marking->length(); i++){
            sum = sum + v->marking->value()[i];
        }

        return (sum + (rand() % distribution_level)) % worker_count;

    }

protected:
    int worker_count;
    int distribution_level;
};

}

#endif // DISTRIBUTIVESUMPARTITIONFUNCTION_H
