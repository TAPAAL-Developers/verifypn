#ifndef PETRICONFIG_H
#define PETRICONFIG_H

#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/PQL/PQL.h"

#include <sstream>

namespace PetriNets {

class PetriConfig : public DependencyGraph::Configuration {

public:
    using Condition = PetriEngine::PQL::Condition;
    PetriConfig() : 
        DependencyGraph::Configuration(), _marking(0), _query(nullptr) 
    {}
    
    PetriConfig(size_t t_marking, Condition *t_query) :
        DependencyGraph::Configuration(), _marking(t_marking), _query(t_query) {
    }

    size_t _marking;
    Condition *_query;

};

}
#endif // PETRICONFIG_H
