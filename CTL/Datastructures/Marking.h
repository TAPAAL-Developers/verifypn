#ifndef MARKING_H
#define MARKING_H

#include "../../PetriEngine/PetriNet.h"

class Marking
{
public:
    Marking(){}

    PetriEngine::MarkVal *marking;
    int length;
};

#endif // MARKING_H
