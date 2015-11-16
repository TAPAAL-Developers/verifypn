#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "../CTLParser/CTLParser.h"
#include "marking.h"
#include <list>

namespace ctl {

//Forward Declaration of edge
class Edge;

class Configuration
{
    enum Assignment {
        CZERO = 2, ONE = 1, ZERO = 0, UNKNOWN = -1
    };
public:
    Configuration(){}
    Configuration(Marking* t_marking, CTLTree* t_query);

    bool operator==(const Configuration& rhs)const;

    inline Marking* marking(){return m_marking;}
    inline CTLTree* Query(){return m_query;}
    std::list<Edge*> DependencySet;
    bool IsNegated = false;
    Assignment assignment = UNKNOWN;

private:

    Marking* m_marking;
    CTLTree* m_query;
};
}// end of ctl

#endif // CONFIGURATION_H
