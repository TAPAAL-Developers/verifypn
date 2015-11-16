#include "dgengine.h"

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

Marking* DGEngine::CreateMarking(const Marking& t_marking, int t_transition){
        Marking* new_marking = new Marking();

        new_marking->CopyMarking(t_marking);

        for(int p = 0; p < _nplaces; p++){
            int place = (*new_marking)[p] - _net->inArc(p,t_transition);
            (*new_marking)[p] = place + _net->outArc(t_transition,p);
        }
        auto pair = Markings.insert(new_marking);

        //*((uset.insert(newConfig)).first)
        return *(pair.first);
    }
}
