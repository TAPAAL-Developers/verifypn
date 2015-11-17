#include "dgengine.h"

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

Configuration* DGEngine::createConfiguration(Marking &t_marking, CTLTree& t_query){
    Configuration* newConfig = new Configuration();

    newConfig->marking = &t_marking;
    newConfig->Query = &t_query;

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->IsNegated = true;
    }

    return *((Configurations.insert(newConfig)).first);
}

Marking* DGEngine::createMarking(const Marking& t_marking, int t_transition){
        Marking* new_marking = new Marking();

        new_marking->CopyMarking(t_marking);

        for(int p = 0; p < _nplaces; p++){
            int place = (*new_marking)[p] - _net->inArc(p,t_transition);
            (*new_marking)[p] = place + _net->outArc(t_transition,p);
        }


        return *(Markings.insert(new_marking).first);
    }
}
