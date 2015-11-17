#include "dgengine.h"
#include "../CTLParser/CTLParser.h"

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

//std::vector<Edge*> DGEngine::successors(Configuration& v) {
//    std::vector<Edge*> W;
//    if(v.query->quantifier == A){
//        if(v.query->path == U){
//            Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
//            Edge* e = new Edge();
//            Edge* e1 = new Edge();
//            e->source = &v;
//            e->targets.push_back(c);
//            W.push_back(e);

//            e1->source = &v;
//            Configuration* b = createConfiguration(*v.marking, *v.query->first);
//            e1->targets.push_back(b);
//            int i = 0;
//            while(true){
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                    i++;
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e1.targets.push_back(c1);

//                } else break;
//            }
//            #ifdef PP
//            edgePrinter(e);edgePrinter(e1);
//            #endif
//            if(i > 0){
//             W.push_back(e1);
//            }


//        } else if(v.query->path == X){
//            bool nxt_s = false;
//            Edge e;
//            e.source = v;
//            while(true){
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                nxt_s = true;
//                Configuration c1 = createConfiguration(nxt_m, v.query->first);
//                e.targets.push_back(c1);

//                } else break;
//            }
//            if(nxt_s){ W.push_back(e);}
//        } else if(v.query->path == F){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            bool nxt_s = false;
//            Edge e, e1;
//            e.source = v;
//            e.targets.push_back(c);

//                W.push_back(e);
//            e1.source = v;
//            while(true){
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                nxt_s = true;
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e1.targets.push_back(c1);

//                } else break;
//            }
//            if(nxt_s){

//                W.push_back(e1);
//            }
//        } else if (v.query->path == G){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            Edge e;
//            e.source = v;
//            e.targets.push_back(c);
//            while(true){
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e.targets.push_back(c1);
//                } else break;
//            }
//               W.push_back(e);
//        }
//    } else if (v.query->quantifier == E){
//        if(v.query->path == U){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            Configuration c1 = createConfiguration(v.marking, v.query->second);
//            Edge e;
//            e.source = v;
//            e.targets.push_back(c1);

//            while(true){
//            int i = 0;
//            Edge e1;
//            e1.source = v;
//            e1.targets.push_back(c);
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                i++;
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e1.targets.push_back(c1);
//                } else break;
//            if(i > 0){ W.push_back(e1); }
//            }
//            W.push_back(e);
//        } else if(v.query->path == X){
//            while(true){
//            bool nxt_s = false;
//            Edge e1;
//            e1.source = v;
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                nxt_s = true;
//                Configuration c1 = createConfiguration(nxt_m, v.query->first);
//                e1.targets.push_back(c1);
//                } else break;
//            if(nxt_s){  W.push_back(e1); }
//            }
//        } else if(v.query->path == F){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            Edge e;
//            e.source = v;
//            e.targets.push_back(c);

//            while(true){
//            bool nxt_s = false;
//            Edge e1;
//            e1.source = v;
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                nxt_s = true;
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e1.targets.push_back(c1);
//                } else break;
//            if(nxt_s){ W.push_back(e1); }

//            }
//            W.push_back(e);
//        } else if(v.query->path == G){
//            int i = 0;
//            while(true){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            Edge e;
//            e.source = v;
//            e.targets.push_back(c);
//            PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
//                if(next_state(v.marking, nxt_m) == 1){
//                Configuration c1 = createConfiguration(nxt_m, v.query);
//                e.targets.push_back(c1);
//                } else { if (i == 0)  W.push_back(e); break;  }
//            i++;
//            W.push_back(e);
//            }
//        }
//    } else if (v.query->quantifier == AND){
//        Configuration c = createConfiguration(v.marking, v.query->first);
//        Configuration c1 = createConfiguration(v.marking, v.query->second);
//        Edge e;
//        e.source = v;
//        e.targets.push_back(c);
//        e.targets.push_back(c1);
//        W.push_back(e);
//    } else if (v.query->quantifier == OR){
//        Configuration c = createConfiguration(v.marking, v.query->first);
//        Configuration c1 = createConfiguration(v.marking, v.query->second);
//        Edge e;
//        Edge e1;
//        e.source = v;
//        e1.source = v;
//        e.targets.push_back(c);
//        e1.targets.push_back(c1);
//        W.push_back(e);
//        W.push_back(e1);
//    } else if (v.query->quantifier == NEG){
//            Configuration c = createConfiguration(v.marking, v.query->first);
//            Edge e;
//            e.source = v;
//            localSmolka(c);
//            e.targets.push_back(c);
//            W.push_back(e);

//            //Make stuff that goes here
//    } else {
//        if (evaluateQuery(v.marking, v.query)){
//            Edge e;
//            e.source = v;
//            assignConfiguration(v, ONE);
//            Configuration &c = v;
//            e.targets.push_back(c);
//            W.push_back(e);

//        } else {
//            Edge e;
//            e.source = v;
//            assignConfiguration(v, CZERO);
//            Configuration &c = v;
//            e.targets.push_back(c);
//            W.push_back(e);
//      }
//    }
//}

std::vector<Marking*> DGEngine::nextState(Marking& t_marking){

    std::vector<Marking*> nextStates;

    //Calculate possible fireable transistions
    //if necessary
    if(t_marking.possibleTransistions.size() == 0)
        calculateFireableTransistions(t_marking);

    auto iter = t_marking.possibleTransistions.begin();
    auto iterEnd = t_marking.possibleTransistions.end();

    //Create new markings for each possible
    //fireable transistion
    for(; iter != iterEnd; iter++){
        nextStates.push_back(createMarking(t_marking, *iter));
    }

    return nextStates;
}

int DGEngine::calculateFireableTransistions(Marking &t_marking){

    for(int t = 0; t < _ntransitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < _nplaces; p++){
            if(t_marking.possibleTransistions[p] < _net->inArc(p,t))
                transitionFound = false;
        }

        if(transitionFound)
            t_marking.possibleTransistions.push_back(t);
    }
    return t_marking.possibleTransistions.size();
}

Configuration* DGEngine::createConfiguration(Marking &t_marking, CTLTree& t_query){
    Configuration* newConfig = new Configuration();

    newConfig->marking = &t_marking;
    newConfig->query = &t_query;

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->IsNegated = true;
    }

    return *(Configurations.insert(newConfig).first);
}

Marking* DGEngine::createMarking(const Marking& t_marking, int t_transition){
        Marking* new_marking = new Marking();

        new_marking->CopyMarking(t_marking);

        for(int p = 0; p < _nplaces; p++){
            int place = (*new_marking)[p] - _net->inArc(p,t_transition);
            (*new_marking)[p] = place + _net->outArc(t_transition,p);
        }

        //insert function either inserts or finds the element
        return *(Markings.insert(new_marking).first);
    }
}
