#include "configuration.h"

namespace ctl{

Configuration::Configuration(Marking * t_marking, CTLTree * t_query){
    marking = t_marking;
    query = t_query;
}

Configuration::~Configuration(){
    std::cout << "Destroying Configuration\n" << std::flush;
    for(Edge* e: Successors){
        delete (e);
    }
}

void Configuration::removeSuccessor(Edge *t_successor){
    {
        Successors.remove(t_successor);

        for(auto t : t_successor->targets){
            t->DependencySet.remove(t_successor);
        }

        delete t_successor;
        return;
    }
}


void Configuration::configPrinter(){
    std::cout << "--------------- Configuration Information -------------------\n";
    CTLParser ctlParser = CTLParser();
    int i = 0;
    std::cout << "Configuration marking: " << std::flush;
    marking->print();

    std::cout << "\nConfiguration query::::\n" ;
    ctlParser.printQuery(query);
    std::cout << "\nConfiguration assignment: " << assignment<<"\n";

    std::cout << "\nshould be negated?: " << IsNegated <<"\n";

    
    std::cout << "---------------------------------------------------------\n";
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
