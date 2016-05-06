#ifndef CONFIGURATION_H1
#define CONFIGURATION_H1
//TODO FIX GUARD

#include "Edge.h"

#include <string>
#include <cstdio>
#include <vector>

namespace DependencyGraph {

class Edge;

enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

class Configuration
{
    unsigned int distance;
public:
    typedef std::vector<Edge*> container_type;

    unsigned int getDistance() const { return distance; }
    void setDistance(unsigned int value) { distance = value; }

    Configuration() {}
    virtual ~Configuration() {
        for(Edge *e : successors)
            delete e;
        for(Edge *e : deleted_successors)
            delete e;
    }

    //Removes a single instance of a successor
    //Should not have multiple equal successors
    void removeSuccessor(Edge *t_successor){
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

    virtual void printConfiguration(){
        std::printf("==================== Configuration ====================\n");
        std::printf("Addr: %ld, Assignment: %s, IsNegated: %s\n",
                    (unsigned long int)this,
                    assignmentToStr(assignment).c_str(),
                    is_negated ? "True" : "False" );
        std::printf("=======================================================\n");
    }

    std::string assignmentToStr(Assignment a){
        if(a == ONE)
            return std::string("ONE");
        else if(a == UNKNOWN)
            return std::string("UNKNOWN");
        else if(a == ZERO)
            return std::string("ZERO");
        else
            return std::string("CZERO");
    }


    bool isDone() { return assignment == ONE || assignment == CZERO; }

    Assignment assignment = UNKNOWN;
    container_type successors = container_type(0);
    container_type deleted_successors = container_type(0);
    container_type dependency_set = container_type(0);
    bool is_negated = false;
};


}
#endif // CONFIGURATION_H
