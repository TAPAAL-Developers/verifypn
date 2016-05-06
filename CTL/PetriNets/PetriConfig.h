#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include "../DependencyGraph/Configuration.h"
#include "../../CTLParser/CTLParser.h"
#include "Marking.h"

namespace PetriNets {

class PetriConfig : public DependencyGraph::Configuration {

public:

    PetriConfig(Marking *t_marking, CTLTree *t_query) :
        DependencyGraph::Configuration(), marking(t_marking), query(t_query) {
        setDistance(query->depth);
    }

    Marking *marking;
    CTLTree *query;

    virtual void printConfiguration() override {
        DependencyGraph::Configuration::printConfiguration();
        std::printf("Marking: ");
        marking->print();
        std::printf(" (%ld) ", (unsigned long int) marking);
        std::printf(" Query: ");
        CTLParser p;
        p.printQuery(query);
        std::printf(" (%ld) ", (unsigned long int) query);
        std::printf("\n=======================================================\n");
    }

};

}
#endif // PETRICONFIG_H
