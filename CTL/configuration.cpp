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
        std::cout << "destroying config" << std::endl << std::flush;
        Successors.remove(t_successor);

        for(auto t : t_successor->targets){
            t->DependencySet.remove(t_successor);
        }

        delete t_successor;
        return;
    }
}


void Configuration::configPrinter(){
    CTLParser ctlParser = CTLParser();
    int i = 0;
    std::cout << "Marking: ";
    marking->print();

    std::cout << " Q: " ;
    ctlParser.printQuery(query);
    std::cout << " Assign: " << assignment;

    std::cout << " NEG: " << IsNegated << "\n";
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
