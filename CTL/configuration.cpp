#include "configuration.h"
#include "../CTLParser/CTLParser_v2.h"

namespace ctl{

Configuration::Configuration(Marking * t_marking, CTLQuery * t_query){
    marking = t_marking;
    query = t_query;
}

Configuration::~Configuration(){

    for(Edge* e: Successors){
        delete (e);
    }
    for(Edge* e: deletedSuccessors){
        delete e;
    }
}

void Configuration::removeSuccessor(Edge *t_successor){
    {
        if (t_successor->isDeleted)
            return;
        Successors.remove(t_successor);

        for(auto t : t_successor->targets){
            t->DependencySet.remove(t_successor);
        }

        t_successor->isDeleted = true;
        deletedSuccessors.push_front(t_successor);
        //delete t_successor;
        return;
    }
}


void Configuration::configPrinter(){
    CTLParser_v2 ctlParser = CTLParser_v2();
    int i = 0;
    std::cout << "Marking: ";
    marking->print();
    std::cout << " Q: " << std::flush;
    std::cout << ctlParser.QueryToString(query)<<std::flush;
    std::cout << " D: "<<query->Depth  << std::flush;
    std::cout << " Assign: " << assignment << std::flush;

    std::cout << " NEG: " << IsNegated << "\n" << std::flush;
}

bool Configuration::operator ==(const Configuration& rhs) const{
    if(query != rhs.query)
        return false;
    else if(IsNegated != rhs.IsNegated)
        return false;
    else if(*marking == *(rhs.marking))//Use equality from marking
        return true;
    else
        return false;
}

}
