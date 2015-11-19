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
        this->m_marking = (MarkVal*)malloc(sizeof(MarkVal) * t_marking.Length());
        this->m_length = t_marking.Length();

        for(int i = 0; i < m_length; i++){
            m_marking[i] = t_marking[i];
        }
    }
}

