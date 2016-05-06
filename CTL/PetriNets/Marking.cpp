#include <string.h>
#include <iostream>

#include "Marking.h"

using namespace PetriEngine;

namespace PetriNets {

    bool Marking::operator ==(const Marking& rhs) const {
        for(int i = 0; i < this->length(); i++){
            if(!(this->value()[i] == rhs.value()[i])){
                return false;
            }
        }
        return true;
    }

    void Marking::copyMarking(const Marking& t_marking){
        this->m_marking = (MarkVal*) malloc(sizeof(MarkVal) * t_marking.length());
        this->m_length = t_marking.length();

        for(int i = 0; i < m_length; i++){
            m_marking[i] = t_marking[i];
        }
    }

    void Marking::print() const {
        for(int i = 0; i < m_length; i++){
            std::cout << m_marking[i] <<std::flush;
        }
    }
}

