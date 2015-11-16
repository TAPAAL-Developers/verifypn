#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "../CTLParser/CTLParser.h"
#include "marking.h"
#include <list>
#include <stdint.h>

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

    Marking* marking;
    CTLTree* Query;
    std::list<Edge*> DependencySet;
    bool IsNegated = false;
    Assignment assignment = UNKNOWN;
};
}// end of ctl

namespace std{
template<>
struct hash<ctl::Configuration>{
    size_t operator()(const ctl::Configuration& t_config) const {
        hash<ctl::Marking> hasher;
        size_t seed = (size_t)reinterpret_cast<uintptr_t>(t_config.Query);
        //Combine query ptr adr with marking hashing
        size_t result = hasher.operator ()(*t_config.marking);
        result ^= seed + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return result;
    }
};
}

#endif // CONFIGURATION_H
