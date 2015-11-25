#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include "../CTLParser/CTLParser.h"
#include "marking.h"
#include "edge.h"
#include <list>
#include <stdint.h>

namespace ctl {

enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

class Configuration
{
public:

    struct Configuration_Equal_To{
        bool operator()(const Configuration* rhs, const Configuration* lhs) const{
            return (*rhs)==(*lhs);
        }
    };

    Configuration(){}
    Configuration(Marking* t_marking, CTLTree* t_query);
    virtual ~Configuration();

    void removeSuccessor(Edge *t_successor);
    void configPrinter();

    bool operator==(const Configuration& rhs)const;
    bool operator!=(const Configuration &rhs) const {return !(*this == rhs);}

    Marking* marking;
    CTLTree* query;
    std::list<Edge*> Successors;
    std::list<Edge*> DependencySet;
    bool IsNegated = false;
    
 Assignment assignment = UNKNOWN;
};
}// end of ctl

namespace std{
//Hash specialization implementations
template<>
struct hash<ctl::Configuration>{
    size_t operator()(const ctl::Configuration& t_config) const {
        hash<ctl::Marking> hasher;
        size_t seed = (size_t)reinterpret_cast<uintptr_t>(t_config.query);
        //Combine query ptr adr with marking hashing
        size_t result = hasher.operator ()(*t_config.marking);
        result ^= seed + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return result;
    }
};
template<>
struct hash<ctl::Configuration*>{
    size_t operator()(const ctl::Configuration* t_config) const {
        hash<ctl::Configuration> hasher;
        return hasher.operator ()(*t_config);
    }
};

}

#endif // CONFIGURATION_H