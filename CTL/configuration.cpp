#include "configuration.h"

namespace ctl{

Configuration::Configuration(Marking *t_marking, CTLTree *t_query){
    m_marking = t_marking;
    t_query = m_query;
}

bool Configuration::operator ==(const Configuration& rhs) const{
    if(m_query != rhs.m_query)
        return false;
    else if(assignment != rhs.assignment)
        return false;
    else if(IsNegated != rhs.IsNegated)
        return false;
    else if(*m_marking == *(rhs.m_marking))//Use equality from marking
        return true;
    else
        return false;
}

}
