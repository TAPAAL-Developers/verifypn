#ifndef DISTRIBUTIONRATIOPARTITIONFUNCTION_H
#define DISTRIBUTIONRATIOPARTITIONFUNCTION_H


#include "../Algorithm/PartitionFunction.h"
#include "Marking.h"
#include "PetriConfig.h"
#include <stdint.h>

namespace PetriNets {

class RandomPartitionFunction : public Algorithm::PartitionFunction
{
public:
    RandomPartitionFunction(int worker_count, int percent)
        : worker_count(worker_count), percent(percent) {}

    std::hash<Marking*> hasher;

    virtual int ownerId(DependencyGraph::Configuration *c) override {        
        PetriConfig *v = static_cast<PetriConfig*>(c);
        // _percent is the percent of markings that will be computed distributively
        // I.e. _percent = 90 means that 10% of the markings will be computed by
        // worker 0 and the remaining 90% distributed evenly on the remaining workers
        // _percent >= 100 is the same as the UniformPartitioning
        if (v->marking->partition < 0) {
            srand(hasher(v->marking));
            int d = (rand() % 100) + 1;
            if (d <= percent) {
                return (rand() % (_number_of_workers - 1)) + 1;
            }
            else {
                return 0;
            }
        }
    }

protected:
    int worker_count;
    int percent;
};

}

#endif // DISTRIBUTIONRATIOPARTITIONFUNCTION_H
