#ifndef MARKING_H
#define MARKING_H

#include "../../PetriEngine/PetriNet.h"

class Marking
{
public:
    Marking(int t_length = -1) : length(t_length){

    }

    PetriEngine::MarkVal *marking;
    int length;
};

#endif // MARKING_H
