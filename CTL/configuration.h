#ifndef CONFIGURATION_H
#define CONFIGURATION_H

namespace ctl {

//Forward Decleration
class Edge;
class CTLTree;

class Configuration
{
    enum Assignment {
        CZERO = 2, ONE = 1, ZERO = 0, UNKNOWN = -1
    };
public:
    Configuration(){};
    Configuration(Marking* t_marking, CTLParser::CTLTree* t_query)
        : m_marking(t_marking), m_query(t_query){}

    bool operator==(const Configuration& rhs)const;

    //Return pointers here instead?
    inline Marking& Marking(){return m_marking;}
    inline CTLParser::CTLTree& Query(){return m_query;}

    Assignment assignment = UNKNOWN;
private:
    Marking* m_marking;
    CTLParser::CTLTree* m_query;
};
}// end of ctl

#endif // CONFIGURATION_H
