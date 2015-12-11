#include "local_fp_algorithm.h"

namespace ctl {

Local_FP_Algorithm::Local_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking)
{
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

bool Local_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W)
{
    std::cout << _net->numberOfTransitions() << std::endl << std::flush;
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    _querySatisfied = local_fp_algorithm(*c0, *t_W);

    //Clean up
    delete t_W;

    return _querySatisfied;
}

bool Local_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W, CircleDetector *t_detector)
{
    std::cout << _net->numberOfTransitions() << std::endl << std::flush;
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    _querySatisfied = local_fp_algorithm(*c0, *t_W);

    //Clean up
    delete t_W;
    delete t_detector;

    return _querySatisfied;
}

bool Local_FP_Algorithm::local_fp_algorithm(Configuration &v, EdgePicker &W)
{
    PriorityQueue N;

    v.assignment = ZERO;
    for(Edge *e : successors(v)){
        W.push(e);
        if(e->source->IsNegated)
            N.push(e);
    }

    while(!W.empty() || !N.empty()){
        Edge *e;

        if(!W.empty()) {
            e = W.pop();
            //std::cout << "Popped negation edge from N: \n" << std::flush;
            e->edgePrinter();
        }
        else if (!N.empty()) {
            e = N.top();
            N.pop();
            //std::cout << "Popped negation edge from N: \n" << std::flush;
            e->edgePrinter();
        }

        /*****************************************************************/
        /*Data handling*/
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        int targetUKNOWNassignments = 0;

        for(auto c : e->targets){
            if (c->assignment == ONE) {
                targetONEassignments++;
            }
            else if (c->assignment == ZERO) {
                targetZEROassignments++;
            }
            else if(c-> assignment == UNKNOWN){
                targetUKNOWNassignments++;
            }

        }
        /*****************************************************************/

        if(e->source->assignment == ONE){
            //std::cout << "== Ignored ==\n" << std::flush;
        }
        else if(e->targets.size() == targetONEassignments){
            if(e->source->IsNegated){
                e->source->assignment = ZERO;
            }
            else{
                e->source->assignment = ONE;
                if(e->source == &v) break;

                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
        }
        else if(targetZEROassignments > 0){
            if(e->source->IsNegated && e->processed){
                e->source->assignment = ONE;
                //std::cout << "== Assigned ONE to NEG Edge ==\n" << std::flush;
                if(e->source == &v) break;

                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
            else{
                e->source->DependencySet.push_back(e);
            }
        }
        else if(targetUKNOWNassignments > 0){
            for(Configuration *tc : e->targets){
                if(tc->assignment == UNKNOWN){
                    tc->assignment = ZERO;
                    tc->DependencySet.push_back(e);

                    for(Edge *succ : successors(*tc)){
                        W.push(succ);
                        if(succ->source->IsNegated){
                            N.push(succ);
                        }
                    }
                }
            }
        }
        e->processed = true;
    }

    return v.assignment == ONE ? true : false;
}

}//ctl
