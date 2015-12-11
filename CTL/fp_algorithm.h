#ifndef FP_ALGORITHM_H
#define FP_ALGORITHM_H

#include "marking.h"
#include "configuration.h"
#include "edge.h"
#include "edgepicker.h"
#include "../CTLParser/CTLParser.h"

#include <unordered_set>
#include <list>

namespace ctl{

class FP_Algorithm
{
public:
    virtual bool search(CTLTree *t_query, EdgePicker *t_W) =0;
    virtual bool search(CTLTree *t_query, EdgePicker *t_W, CircleDetector *t_detector) =0;
    void clear(bool t_clear_all = false);
    inline bool querySatisfied(){return _querySatisfied; }
    inline int configuration_count(){return Configurations.size();}
    inline int marking_count() {return Markings.size();}
    volatile int evilCycles = 0;
    volatile int cycles = 0;

protected:
    std::list<Edge *> successors(Configuration &v);
    bool evaluateQuery(Configuration &t_config);
    int indexOfPlace(char *t_place);
    std::list<Marking *> nextState(Marking &t_marking);
    std::list<int> calculateFireableTransistions(Marking &t_marking);
    Configuration *createConfiguration(Marking &t_marking, CTLTree &t_query);
    Marking *createMarking(const Marking &t_marking, int t_transition);

    PetriEngine::PetriNet *_net;
    PetriEngine::MarkVal *_m0;
    int _nplaces;
    int _ntransitions;
    bool _querySatisfied;

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> Markings;

    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Configurations;

};
}//ctl
#endif // FP_ALGORITHM_H
