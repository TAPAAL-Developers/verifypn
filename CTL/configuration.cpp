#include "configuration.h"

namespace ctl{

Configuration::Configuration(Marking * t_marking, CTLTree * t_query){
    marking = t_marking;
    query = t_query;
}

Configuration::~Configuration(){
    for(Edge* e: Successors){
        delete (e);
    }
}

void Configuration::removeSuccessor(Edge *t_successor){
    {
        for(auto e : Successors){
            if(e == t_successor){
                delete e;
                return;
            }
        }
    }
}

bool Configuration::operator ==(const Configuration& rhs) const{
    if(query != rhs.query)
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
