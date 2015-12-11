#ifndef CZERO_FP_ALGORITHM_H
#define CZERO_FP_ALGORITHM_H
#include "edge.h"
#include "configuration.h"
#include "edgepicker.h"
#include "../CTLParser/CTLParser.h"
#include "../PetriEngine/PetriNet.h"

#include <queue>

namespace ctl{

class CZero_FP_Algorithm
{
public:
    struct edge_prioritizer{
        bool operator()(const Edge *lhs, const Edge *rhs) const {
            return (lhs->source->query->max_depth > rhs->source->query->max_depth);
        }
    };

    CZero_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking);
    bool search(CTLTree *t_query, EdgePicker *t_W);
    inline bool querySatisfied(){return _querySatisfied; }
    inline int configuration_count(){return Configurations.size();}
    inline int marking_count() {return Markings.size();}
    void clear(bool t_clear_all = false);
    volatile int evilCircles = 0;
    volatile int circles = 0;

private:
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

    bool czero_fp_algorithm(Configuration &v, EdgePicker &W);

    std::list<Edge *> successors(Configuration &v);
    bool evaluateQuery(Configuration &t_config);
    int indexOfPlace(char *t_place);
    std::list<Marking *> nextState(Marking &t_marking);
    std::list<int> calculateFireableTransistions(Marking &t_marking);
    Configuration *createConfiguration(Marking &t_marking, CTLTree &t_query);
    Marking *createMarking(const Marking &t_marking, int t_transition);

    typedef std::priority_queue<Edge*, std::vector<Edge*>, ctl::CZero_FP_Algorithm::edge_prioritizer> PriorityQueue;
};
}//ctl
#endif // CZERO_FP_ALGORITHM_H
