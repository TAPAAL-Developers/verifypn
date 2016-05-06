#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "../DependencyGraph/AbstractDependencyGraphs.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"
#include "../../CTLParser/CTLParser.h"
#include "PetriConfig.h"
#include "Marking.h"
#include "../../PetriParse/PNMLParser.h"

#include <unordered_set>
#include <list>

namespace PetriNets {

class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph
{
public:
    OnTheFlyDG(PetriEngine::PetriNet *t_net,
               PetriEngine::MarkVal *t_initial,
               PNMLParser::InhibitorArcList t_inhibitorArcs);

    virtual ~OnTheFlyDG();

    virtual void successors(DependencyGraph::Configuration *c) override;
    virtual DependencyGraph::Configuration *initialConfiguration() override;
    virtual void cleanUp() override;

    void setQuery(CTLTree* query);

protected:

    //initialized from constructor
    PetriEngine::PetriNet *net;
    PNMLParser::InhibitorArcList inhibitorArcs;
    Marking *initial_marking = nullptr;
    int n_transitions = 0;
    int n_places = 0;

    //used after query is set
    DependencyGraph::Configuration *initial = nullptr;
    std::vector<Marking*> cached_successors;
    Marking *cached_marking = nullptr;
    CTLTree *query = nullptr;

    bool evaluateQuery(CTLTree &query, Marking &marking);
    bool fastEval(CTLTree &query, Marking &marking);
    std::vector<Marking*> nextState(Marking &marking);
    int indexOfPlace(char *t_place);
    std::list<int> calculateFireableTransistions(Marking &marking);
    DependencyGraph::Configuration *createConfiguration(Marking &marking, CTLTree &query);
    Marking *createMarking(const Marking &marking, int t_transition);

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> markings;

};
}
#endif // ONTHEFLYDG_H
