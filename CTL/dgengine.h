#ifndef DGENGINE_H
#define DGENGINE_H

#include <unordered_set>

#include "marking.h"
#include "configuration.h"
#include "edge.h"
#include "../PetriEngine/PetriNet.h"

namespace ctl{

class DGEngine
{
public:
    DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]);

    bool querySatisfied = false;
private:
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    int _nplaces;
    int _ntransitions;

    std::unordered_set<Marking*, std::hash<Marking*>, Marking::Marking_Equal_To> Markings;
    //std::unordered_set<Configuration*> Configurations;
    //std::unordered_set<Edge*> Edges;

    Marking* CreateMarking(const Marking& t_marking, int t_transition);

};
}
#endif // DGENGINE_H
