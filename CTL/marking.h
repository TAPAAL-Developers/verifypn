#ifndef MARKING_H
#define MARKING_H

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
    Marking(){}
    Marking(PetriEngine::MarkVal* t_marking, size_t t_length)
        : m_marking(t_marking), m_length(t_length){}

    ~Marking(){free(m_marking);}

    void CopyMarking(const Marking& t_marking);
    void CopyMarking(const PetriEngine::MarkVal t_markvals[], const size_t t_length);

    bool operator==(const Marking& rhs) const;

    //Hopefully the compiler with agree with this inlining
    inline PetriEngine::MarkVal operator[](const int index) const {
        return m_marking[index];
    }
    inline PetriEngine::MarkVal* Value() const {return m_marking;}
    inline size_t Length() const {return m_length;}
private:
    PetriEngine::MarkVal* m_marking;
    size_t m_length;
};
}
#endif // MARKING_H
