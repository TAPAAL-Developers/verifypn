#ifndef FEATURED_PETRICONFIG_H
#define FEATURED_PETRICONFIG_H

#include "FCTL/DependencyGraph/FConfiguration.h"
#include "PetriEngine/PQL/PQL.h"

#include <sstream>

namespace Featured {
    namespace PetriNets {

        class PetriConfig : public DependencyGraph::Configuration {

        public:
            using Condition = PetriEngine::PQL::Condition;

            PetriConfig() :
                    DependencyGraph::Configuration(), marking(0), query(NULL) {}

            PetriConfig(size_t t_marking, Condition* t_query) :
                    DependencyGraph::Configuration(), marking(t_marking), query(t_query) {
            }

            size_t marking;
            Condition* query;

        };
    }
}
#endif // PETRICONFIG_H
