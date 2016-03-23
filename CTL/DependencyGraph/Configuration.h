#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <string>
#include <cstdio>
#include <vector>

namespace DependencyGraph {

class Edge;

enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

std::string assignment_to_str(Assignment a){
    if(a == ONE)
        return std::string("ONE");
    else if(a == UNKNOWN)
        return std::string("UNKNOWN");
    else if(a == ZERO)
        return std::string("ZERO");
    else
        return std::string("CZERO");
}

class Configuration
{
public:
    typedef std::vector<Edge*> container_type;

    Configuration() {}
    ~Configuration() {
        for(Edge *e : successors)
            delete e;
        for(Edge *e : deletedSuccessors)
            delete e;
    }

    //Removes a single instance of a successor
    //Should not have multiple equal successors
    void removeSuccessor(Edge *t_successor){
        auto iter = successors.begin();
        auto end = successors.end();

        while(iter != end){
            if(*iter == t_successor){
                deletedSuccessors.insert(deletedSuccessors.end(), *iter);
                successors.erase(iter);
                successors.shrink_to_fit();
                deletedSuccessors.shrink_to_fit();
                break;
            }
            else
                iter++;
        }
    }

    virtual void printConfiguration(){
        std::printf("==================== Configuration ====================\n");
        std::printf("Addr: %ld, Assignment: %s, IsNegated: %s\n",
                    (unsigned long int)this,
                    assignment_to_str(assignment).c_str(),
                    IsNegated ? "True" : "False" );
        std::printf("=======================================================\n");
    }

    Assignment assignment = UNKNOWN;
    container_type successors = container_type(0);
    container_type deletedSuccessors = container_type(0);
    container_type DependencySet = container_type(0);
    bool IsNegated = false;
};

}
#endif // CONFIGURATION_H
