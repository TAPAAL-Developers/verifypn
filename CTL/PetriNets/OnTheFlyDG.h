#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "../DependencyGraph/AbstractDependencyGraphs.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"
#include "../Communicator/Serializer.h"
#include "../../CTLParser/CTLQuery.h"
#include "PetriConfig.h"
#include "Marking.h"
#include "../../PetriParse/PNMLParser.h"

#include <unordered_set>
#include <list>

namespace PetriNets {

class OnTheFlyDG : public DependencyGraph::BasicDependencyGraph, public Serializer
{
public:
    OnTheFlyDG(PetriEngine::PetriNet *t_net,
               PetriEngine::MarkVal *t_initial,
               PNMLParser::InhibitorArcList t_inhibitorArcs);

    virtual ~OnTheFlyDG();

    //Dependency graph interface
    virtual void successors(DependencyGraph::Configuration *c) override;
    virtual DependencyGraph::Configuration *initialConfiguration() override;
    virtual void cleanUp() override;

    //Serializer interface
    virtual std::pair<int, int*> serialize(SearchStrategy::Message &m) override;
    virtual SearchStrategy::Message deserialize(int* message, int messageSize) override;

    void setQuery(CTLQuery* query);

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
    CTLQuery *query = nullptr;

    bool evaluateQuery(CTLQuery &query, Marking &marking);
    bool fastEval(CTLQuery &query, Marking &marking);
    std::vector<Marking*> nextState(Marking &marking);
    bool EvalCardianlity(int a, LoperatorType lop, int b);
    int GetParamValue(CardinalityParameter *param, Marking& marking);
    std::list<int> calculateFireableTransistions(Marking &marking);
    DependencyGraph::Configuration *createConfiguration(Marking &marking, CTLQuery &query);
    Marking *createMarking(const Marking &marking, int t_transition);

    CTLQuery *findQueryById(int id, CTLQuery *root);

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> markings;

};
}
#endif // ONTHEFLYDG_H
