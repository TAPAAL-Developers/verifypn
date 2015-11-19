#ifndef DGENGINE_H
#define DGENGINE_H

#include <unordered_set>
#include <vector>
#include <list>

#include "marking.h"
#include "configuration.h"
#include "edge.h"
#include "../PetriEngine/PetriNet.h"

namespace ctl{

class DGEngine
{
public:
    DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool t_CZero);

    bool querySatisfied = false;
    
    void RunEgineTest(PetriEngine::PetriNet net, PetriEngine::MarkVal m0);
private:
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    int _nplaces;
    int _ntransitions;
    bool _CZero;

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> Markings;

    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Configurations;
    //std::unordered_set<Edge*> Edges;

    std::list<Edge*> successors(Configuration& v);

    bool evaluateQuery(Configuration& t_config);
    int indexOfPlace(char* t_place);

    void assignConfiguration(Configuration& t_config, Assignment t_assignment);
    std::list<Marking*> nextState(Marking& t_marking);
    std::list<int> calculateFireableTransistions(Marking& t_marking);
    Marking* createMarking(const Marking& t_marking, int t_transition);
    Configuration* createConfiguration(Marking& t_marking, CTLTree& t_query);

};
}
#endif // DGENGINE_H
