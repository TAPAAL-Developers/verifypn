#include "configuration.h"

namespace ctl{

Configuration::Configuration(Marking * t_marking, CTLTree * t_query){
    marking = t_marking;
    Query = t_query;
}

bool Configuration::operator ==(const Configuration& rhs) const{
    if(Query != rhs.Query)
        return false;
    else if(assignment != rhs.assignment)
        return false;
    else if(IsNegated != rhs.IsNegated)
        return false;
    else if(*marking == *(rhs.marking))//Use equality from marking
        return true;
    else
        return false;
}

}
