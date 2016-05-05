#include "DependencyGraph.h"
#include <iostream>

namespace ctl{

DependencyGraph::DependencyGraph(PetriEngine::PetriNet *t_net, PetriEngine::MarkVal *t_initial, PNMLParser::InhibitorArcList inhibitorArcs) :
    _petriNet(t_net),
    _initialMarking(t_initial),
    _inhibitorArcs(inhibitorArcs),
    _nplaces(t_net->numberOfPlaces()),
    _ntransitions(t_net->numberOfTransitions()){}

void DependencyGraph::initialize(CTLTree &t_query){
    if(_query != NULL)
        clear(false);
    _query = &t_query;
}


}//ctl
