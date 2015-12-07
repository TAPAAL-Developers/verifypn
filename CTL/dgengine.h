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

//enum Search_Strategy { LOCALSMOLKA, LOCALSMOLKA_BFS, GLOBALSMOLKA, GLOBALSMOLKA_BFS };
enum ctl_algorithm {Local, Global, CZero};
enum ctl_search_strategy {CTL_BFS,              //Breadth-First Search
                          CTL_FBFS,             //BFS + propagation first
                          CTL_BBFS,             //BFS + propagation last
                          CTL_DFS,              //Depth-First Search
                          CTL_BDFS,             //DFS + propagation last
                          CTL_BestFS,           //Heuristic Search
                          CTL_CDFS};            //DFS with circle dection

class DGEngine
{
public:
    DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]);

    void RunEgineTest();
    void search(CTLTree* t_query, ctl_algorithm t_algorithm, ctl_search_strategy t_strategy);
    inline bool querySatisfied(){return _querySatisfied; }
    inline int configuration_count(){return Configurations.size();}
    inline int marking_count() {return Markings.size();}
    void clear(bool t_clear_all = false);
    volatile int evilCircles;
    volatile int circles;
private:

    enum ColourCode {
        FG_BLACK    = 30,
        FG_RED      = 31,
        FG_GREEN    = 32,
        FG_YELLOW   = 33,
        FG_BLUE     = 34,
        FG_PURPLE   = 35,
        FG_CYAN     = 36,
        FG_WHITE    = 37,
        FG_DEFAULT  = 39,
    };

    ctl_search_strategy _strategy;
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

    void colorprinter(std::string str, ColourCode cc);

    std::list<Edge*> detectCircle(Configuration *t_source, Configuration *t_target);
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
