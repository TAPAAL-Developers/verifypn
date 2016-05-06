#include "Configuration.h"

DependencyGraph::Configuration::~Configuration() {
    for(Edge *e : successors)
        delete e;
    for(Edge *e : deleted_successors)
        delete e;
}

void DependencyGraph::Configuration::removeSuccessor(DependencyGraph::Edge *t_successor){
    auto iter = successors.begin();
    auto end = successors.end();

    while(iter != end){
        if(*iter == t_successor){
            deleted_successors.insert(deleted_successors.end(), *iter);
            successors.erase(iter);
            successors.shrink_to_fit();
            deleted_successors.shrink_to_fit();
            break;
        }
        else
            iter++;
    }
}

void DependencyGraph::Configuration::printConfiguration(){
    std::printf("==================== Configuration ====================\n");
    std::printf("Addr: %ld, Assignment: %s, IsNegated: %s\n",
                (unsigned long int)this,
                assignmentToStr(assignment).c_str(),
                is_negated ? "True" : "False" );
    std::printf("=======================================================\n");
}

std::__cxx11::string DependencyGraph::Configuration::assignmentToStr(DependencyGraph::Assignment a){
    if(a == ONE)
        return std::string("ONE");
    else if(a == UNKNOWN)
        return std::string("UNKNOWN");
    else if(a == ZERO)
        return std::string("ZERO");
    else
        return std::string("CZERO");
}

