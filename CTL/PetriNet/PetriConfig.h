#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include "../DependencyGraph/Configuration.h"
#include "../../CTLParser/CTLParser.h"
#include "Marking.h"

namespace PetriNet {

class PetriConfig : public DependencyGraph::Configuration {

public:

    PetriConfig(Marking *t_marking, CTLTree *t_query) :
        DependencyGraph::Configuration(), marking(t_marking), query(t_query) {}

    Marking *marking;
    CTLTree *query;

};

}
#endif // PETRICONFIG_H
