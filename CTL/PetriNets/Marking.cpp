#include <string.h>
#include <sstream>
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

    std::string Marking::toString() const
    {
        std::stringstream ss;
        ss << "Marking (" << this << "): ";
        for(int i = 0; i < m_length; i++)
            ss << m_marking[i];

        return ss.str();
    }

    void Marking::print() const {
        std::cout << toString() << std::endl;
    }
}

