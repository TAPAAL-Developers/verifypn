#ifndef ONTHEFLYDG_H
#define ONTHEFLYDG_H

#include "DependencyGraph.h"
#include "marking.h"
#include "configuration.h"
#include "edge.h"

#include <boost/unordered_set.hpp>
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


    boost::unordered_set< Marking*,
                        boost::hash<Marking*>,
                        Marking::Marking_Equal_To> Markings;

    boost::unordered_set< Configuration*,
                        boost::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Configurations;
    bool isCompressing() {return _compressoption;};
                        
protected:
    bool evaluateQuery(Configuration &t_config);
    bool _compressoption;
    int indexOfPlace(char *t_place);
    std::list<Marking *> nextState(Marking &t_marking);
    std::list<int> calculateFireableTransistions(Marking &t_marking);
    Configuration *createConfiguration(Marking &t_marking, CTLTree &t_query);
    Marking *createMarking(const Marking &t_marking, int t_transition);
    void initCompressOption();

};
}
#endif // ONTHEFLYDG_H
