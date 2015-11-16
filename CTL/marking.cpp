#include <string.h>
#include <cstdlib>

#include "../PetriEngine/PetriNet.h"

#include "marking.h"

using namespace PetriEngine;

namespace ctl {

    bool Marking::operator ==(const Marking& rhs) const {
        return !memcmp(this->Value(), rhs.Value(), this->Length());
    }

    void Marking::CopyMarking(const Marking& t_marking){
        CopyMarking(t_marking.Value(), t_marking.Length());
    }

    void Marking::CopyMarking(const MarkVal t_markvals[], const size_t t_length){
        this->m_marking = (MarkVal*)malloc(sizeof(MarkVal) * t_length);
        this->m_length = t_length;
        memcpy(this->m_marking, t_markvals, this->m_length);
    }

    //Hashing function for marking
//    template<>
//    struct hash<ctl::Marking>{
//        size_t operator () (ctl::Marking t_marking) const {

//            size_t seed = 0x9e3779b9;

//            for(int i = 0; i < t_marking.Length(); i++){
//                seed ^= t_marking[i] + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//            }

//            return seed;
//        }
//    };
}
