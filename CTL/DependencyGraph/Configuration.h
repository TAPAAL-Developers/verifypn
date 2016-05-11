#ifndef CONFIGURATION_H
#define CONFIGURATION_H

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
    unsigned int distance = 0;
public:
    typedef std::vector<Edge*> container_type;

    unsigned int getDistance() const { return distance; }
    void setDistance(unsigned int value) { distance = value; }

    Configuration() {}
    virtual ~Configuration();

    //Removes a single instance of a successor
    //Should not have multiple equal successors
    void removeSuccessor(Edge *t_successor);
    virtual void printConfiguration();
    std::string assignmentToStr(Assignment a);
    bool isDone() const { return assignment == ONE || assignment == CZERO; }

    Assignment assignment = UNKNOWN;
    container_type successors = container_type(0);
    container_type deleted_successors = container_type(0);
    container_type dependency_set = container_type(0);
    bool is_negated = false;
};


}
#endif // CONFIGURATION_H
