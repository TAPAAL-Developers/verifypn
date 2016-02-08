#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "DependencyGraph.h"
#include "marking.h"
#include "configuration.h"
#include "edge.h"

#include <unordered_set>
#include <list>

namespace ctl {

class OnTheFlyDG : public DependencyGraph
{
public:
    OnTheFlyDG(PetriEngine::PetriNet *t_net,
               PetriEngine::MarkVal *t_initial);

    // DependencyGraph interface
    std::list<Edge *> successors(Configuration &v);
    Configuration &initialConfiguration();
    void clear(bool t_clear_all);
    int configuration_count() { return Configurations.size();}
    int marking_count() { return Markings.size(); }

protected:
    bool evaluateQuery(Configuration &t_config);
    int indexOfPlace(char *t_place);
    std::list<Marking *> nextState(Marking &t_marking);
    std::list<int> calculateFireableTransistions(Marking &t_marking);
    Configuration *createConfiguration(Marking &t_marking, CTLTree &t_query);
    Marking *createMarking(const Marking &t_marking, int t_transition);

    std::unordered_set< Marking*,
                        std::hash<Marking*>,
                        Marking::Marking_Equal_To> Markings;

    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Configurations;
};
}
#endif // ONTHEFLYDG_H
