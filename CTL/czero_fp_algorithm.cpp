#include "czero_fp_algorithm.h"

#include <string.h>

namespace ctl{

CZero_FP_Algorithm::CZero_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking)
{
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

bool CZero_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W)
{
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    _querySatisfied = czero_fp_algorithm(*c0, *t_W);

    //Clean up
    delete t_W;

    return _querySatisfied;
}

bool CZero_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W, CircleDetector *t_detector)
{
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    _querySatisfied = czero_fp_algorithm(*c0, *t_W, true, t_detector);

    cycles = t_detector->circles;
    evilCycles = t_detector->evilCircles;

    //Clean up
    delete t_W;
    delete t_detector;

    return _querySatisfied;
}

bool CZero_FP_Algorithm::czero_fp_algorithm(Configuration &v, EdgePicker &W, bool cycle_detection, CircleDetector *detector)
{    
    bool CycleDetection = cycle_detection;
    //CircleDetector detector;
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
            if(CycleDetection)
                detector->push(e);
            //std::cout << "Popped negation edge from N: \n" << std::flush;
            //e->edgePrinter();
        }
        else if (!N.empty()) {
            CycleDetection = false;
            e = N.top();
            N.pop();
            //std::cout << "Popped negation edge from N: \n" << std::flush;
            //e->edgePrinter();
        }

        /*****************************************************************/
        /*Data handling*/
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        int targetUKNOWNassignments = 0;
        bool czero = false;

        for(auto c : e->targets){
            if (c->assignment == ONE) {
                targetONEassignments++;
            }
            else if (c->assignment == ZERO) {
                targetZEROassignments++;
            }
            else if (c->assignment == CZERO){
                czero = true;
                break;
            }
            else if(c-> assignment == UNKNOWN){
                targetUKNOWNassignments++;
            }

        }
        /*****************************************************************/

        if(e->isDeleted || e->source->assignment == ONE || e->source->assignment == CZERO){
            //std::cout << "== Ignored ==\n" << std::flush;
        }
        /*****************************************************************/

        else if(e->targets.size() == targetONEassignments){

            if(e->source->IsNegated){
                e->source->assignment = CZERO;
                e->source->removeSuccessor(e);
            }
            else{
                e->source->assignment = ONE;
            }
            if(e->source == &v) break;

            for(Edge *de : e->source->DependencySet){
                W.push_dependency(de);
            }
            e->source->DependencySet.clear();
        }
        else if(czero){
            if(e->source->IsNegated){
                e->source->assignment = ONE;
                if(e->source == &v) break;

                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
            else{
                if(e->source->Successors.size() <= 1){
                    e->source->assignment == CZERO;
                    if(e->source == &v) break;

                    for(Edge *de : e->source->DependencySet){
                        W.push_dependency(de);
                    }
                    e->source->DependencySet.clear();
                }
            }
            e->source->removeSuccessor(e);
        }
        else if(targetZEROassignments > 0){
            if(e->source->IsNegated && e->processed){
                e->source->assignment = ONE;
                if(e->source == &v) break;
                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
            else {
                for(auto c : e->targets){
                    if(c->assignment == ZERO) {
                        c->DependencySet.push_back(e);
                    }
                }
            }
        }
        else if(targetUKNOWNassignments > 0){
            for(Configuration *tc : e->targets){
                if(tc->assignment == UNKNOWN){
                    tc->assignment = ZERO;
                    tc->DependencySet.push_back(e);
                    successors(*tc);

                    if(tc->Successors.empty()){
                        tc->assignment = CZERO;
                    //    W.push_dependency(e);
                    }
                    else {
                        for(Edge *succ : tc->Successors){
                            W.push(succ);
                            if(succ->source->IsNegated){
                                N.push(succ);
                            }
                        }
                    }
                }
            }
        }
        e->processed = true;
    }

    //std::cout << "Final Assignment: " << v.assignment << " " << ((v.assignment == ONE) ? true : false) << std::endl;
    return (v.assignment == ONE) ? true : false;
}
}//ctl
