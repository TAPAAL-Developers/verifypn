#ifndef MARKING_H
#define MARKING_H

#include <vector>
#include <boost/functional/hash/hash.hpp>
#include "../PetriEngine/PetriNet.h"

namespace ctl {

//A Marking is essentially just an Array.
//However since C++ does not support
//VLA we must make use of a variable to
//said length.
//Along with that we are going make a few
//inline functions to make the class
//act/look/feel like an array.

class Configuration;

class Marking
{
    typedef std::vector<Configuration*> succ_container_type;
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
        : m_marking(t_marking), m_length(t_length){ }

    virtual ~Marking(){ free(m_marking); }

    void CopyMarking(const Marking& t_marking);

    bool operator==(const Marking& rhs) const;

    //Hopefully the compiler with agree with this inlining
    inline PetriEngine::MarkVal& operator[](const int index) const {
        return m_marking[index];
    }
    inline PetriEngine::MarkVal* Value() const {return m_marking;}
    void print() const;
    inline size_t Length() const {return m_length;}
    succ_container_type successors;
private:
    PetriEngine::MarkVal* m_marking;
    size_t m_length;
};
}

namespace boost {
    // Specializations of hash functions.
    // Normal
    template<>
    struct hash<ctl::Marking>{
        size_t operator()(const ctl::Marking& t_marking) const{
            size_t hash = 0;
            uint32_t& h1 = ((uint32_t*)&hash)[0];
            uint32_t& h2 = ((uint32_t*)&hash)[1];
            uint32_t cnt = 0;
            for (size_t i = 0; i < t_marking.Length(); i++)
            {
                if(t_marking[i])
                {
                    h1 ^= 1 << (i % 32);
                    h2 ^= t_marking[i] << (cnt % 32);
                    ++cnt;
                }
            }
            return hash;
        }

        ///Old Hash Function
//        size_t operator()(const ctl::Marking& t_marking ) const {
//            size_t seed = 0x9e3779b9;

//            for(int i = 0; i < t_marking.Length(); i++){
//                seed ^= t_marking[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            }

//            return seed;
//        }
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
