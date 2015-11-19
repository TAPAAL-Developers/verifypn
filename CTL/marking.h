#ifndef MARKING_H
#define MARKING_H

#include <vector>
#include <iostream> 
#include "../PetriEngine/PetriNet.h"

namespace ctl {

//A Marking is essentially just an Array.
//However since C++ does not support
//VLA we must make use of a variable to
//said length.
//Along with that we are going make a few
//inline functions to make the class
//act/look/feel like an array.

class Marking
{
    public:
    // Equality checker for containers
    // with pointers to markings
    struct Marking_Equal_To{
        bool operator()(const Marking* rhs, const Marking* lhs) const{
            return (*rhs)==(*lhs);
        }
    };

    Marking(){}
    Marking(PetriEngine::MarkVal* t_marking, size_t t_length)
        : m_marking(t_marking), m_length(t_length){}

    virtual ~Marking(){free(m_marking);}

    void CopyMarking(const Marking& t_marking);

    bool operator==(const Marking& rhs) const;

    //Hopefully the compiler with agree with this inlining
    inline PetriEngine::MarkVal& operator[](const int index) const {
        return m_marking[index];
    }
    inline PetriEngine::MarkVal* Value() const {return m_marking;}
    inline void print() const {
        for(int i = 0; i < m_length; i++){
            std::cout << m_marking[i] <<std::flush;
        }
    }
    inline size_t Length() const {return m_length;}
private:
    PetriEngine::MarkVal* m_marking;
    size_t m_length;
};
}

namespace std{
    // Specializations of hash functions.
    // Normal
    template<>
    struct hash<ctl::Marking>{
        size_t operator()(const ctl::Marking& t_marking ) const {
            size_t seed = 0x9e3779b9;

            for(int i = 0; i < t_marking.Length(); i++){
                seed ^= t_marking[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }

            return seed;
        }
    };
    // Pointer to Marking
    template<>
    struct hash<ctl::Marking*>{
        size_t operator()(const ctl::Marking* t_marking ) const {
            hash<ctl::Marking> hasher;
            return hasher.operator ()(*t_marking);
        }
    };
}
#endif // MARKING_H
