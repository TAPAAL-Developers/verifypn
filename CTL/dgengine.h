#ifndef DGENGINE_H
#define DGENGINE_H

#include <unordered_set>
#include <vector>
#include <list>
//#include <queue>
//#include <stack>

#include "marking.h"
#include "configuration.h"
#include "edge.h"
#include "../PetriEngine/PetriNet.h"

namespace ctl{

class EdgePicker;

enum Search_Strategy { LOCALSMOLKA, LOCALSMOLKA_BFS, GLOBALSMOLKA, GLOBALSMOLKA_BFS };

class DGEngine
{
public:
    DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool t_CZero);

    void RunEgineTest();
    void search(CTLTree* t_query, Search_Strategy t_strategy);
    inline bool querySatisfied(){return _querySatisfied; }
private:
    Search_Strategy _strategy;
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    int _nplaces;
    int _ntransitions;
    bool _CZero;
    bool _querySatisfied = false;

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> Markings;

    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Configurations;

    bool localSmolka(Configuration& v);
    bool globalSmolka(Configuration& v);

    std::list<Edge*> successors(Configuration& v);
    void CalculateEdges(Configuration &v, EdgePicker &W);
    void buildDependencyGraph(Configuration &v);

    bool evaluateQuery(Configuration& t_config);
    int indexOfPlace(char* t_place);

    void assignConfiguration(Configuration* t_config, Assignment t_assignment);
    std::list<Marking*> nextState(Marking& t_marking);
    std::list<int> calculateFireableTransistions(Marking& t_marking);
    Marking* createMarking(const Marking& t_marking, int t_transition);
    Configuration* createConfiguration(Marking& t_marking, CTLTree& t_query);

    std::vector<char> buffercreator(bool fire, bool simple);
};
}
#endif // DGENGINE_H
