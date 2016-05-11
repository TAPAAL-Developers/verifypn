#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include "../DependencyGraph/Configuration.h"
#include "../../CTLParser/CTLQuery.h"
#include "../../CTLParser/CTLParser_v2.h"
#include "Marking.h"

namespace PetriNets {

class PetriConfig : public DependencyGraph::Configuration {

public:

    PetriConfig(Marking *t_marking, CTLQuery *t_query) :
        DependencyGraph::Configuration(), marking(t_marking), query(t_query) {
    }

    Marking *marking;
    CTLQuery *query;

    virtual void printConfiguration() override {
        DependencyGraph::Configuration::printConfiguration();
        std::printf("Marking: ");
        marking->print();
        std::printf(" (%ld) ", (unsigned long int) marking);
        CTLParser_v2 p;
        std::printf(" Query: %s", p.QueryToString(query));
        std::printf(" (%ld) ", (unsigned long int) query);
        std::printf("\n=======================================================\n");
    }

};

}
#endif // PETRICONFIG_H
