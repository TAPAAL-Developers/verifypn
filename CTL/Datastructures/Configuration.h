#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "BaseConfiguration.h"
#include "Marking.h"
#include "../../CTLParser/CTLParser.h"

namespace ctl {

class Configuration : public ctl::BaseConfiguration
{
public:
    Configuration(){}
    Configuration(Marking *t_marking, CTLTree *t_query = NULL) : marking(t_marking), query(t_query){}

    Marking *marking;
    CTLTree *query;
};
}
#endif // CONFIGURATION_H
