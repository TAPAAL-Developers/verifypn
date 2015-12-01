#ifndef EDGE_H
#define EDGE_H

#include <list>
#include <iostream>
namespace ctl {

//To avoid circular dependency
class Configuration;

class Edge
{
public:
    class EdgePointerEqual{
        bool operator()(const Edge& lhs, const Edge& rhs) const{
            return &lhs == &rhs;
        }
    };
    class EdgePointerHash{
        size_t operator()(const Edge* e)const{
            return (size_t)e;
        }
    };

    Edge(){};
    Edge(Configuration* t_source) : source(t_source) {}
    //virtual ~Edge(){std::cout << "Destroying Edge" << std::endl;}

    bool processed = false;
    //bool isDeleted = false;
    int Rating = -1;
    bool isDeleted = false;
    Configuration* source;
    std::list<Configuration*> targets;

    //bool operator==(const Edge & rhs) const;
    void edgePrinter();
    void rateEdge();
};
}

#endif // EDGE_H
