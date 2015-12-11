#include "global_fp_algorithm.h"

namespace ctl {

Global_FP_Algorithm::Global_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking)
{
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

bool Global_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W)
{
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    buildDependencyGraph(*c0);
    _querySatisfied = global_fp_algorithm(*c0, *t_W);

    //Clean up
    delete t_W;

    return _querySatisfied;
}

bool Global_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W, CircleDetector *t_detector)
{
    std::cout << "Strategy Error: Global Fixed Point Algorithm does not support circle detection\n";
    exit(EXIT_FAILURE);
}


bool Global_FP_Algorithm::global_fp_algorithm(Configuration &v, EdgePicker &W, bool cycle_detection, CircleDetector *detector)
{
    PriorityQueue N;

    CalculateEdges(v, W);

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

                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
        }
        else if(targetZEROassignments > 0){
            if(e->source->IsNegated && e->processed){
                e->source->assignment = ONE;

                for(Edge *de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
            else{
                for(auto c : e->targets){
                    if(c->assignment == ZERO) {
                        c->DependencySet.push_back(e);
                    }
                }
            }
        }

        if(e->source->IsNegated && !e->processed)
            N.push(e);

        e->processed = true;
    }
    return (v.assignment == ONE) ? true : false;
}

void Global_FP_Algorithm::buildDependencyGraph(Configuration &v){
    std::queue<Configuration*> C;
    v.assignment = ZERO;
    C.push(&v);

    //    Make dependency graph
    //    std::cout << "==========================================================" << std::endl;
    //    std::cout << "======================= Start Global =====================" << std::endl;
    //    std::cout << "==========================================================" << std::endl;
    while(!C.empty()){
        Configuration* c = C.front();
        C.pop();

        successors(*c);

        for(Edge* e : c->Successors){
 //           e->edgePrinter();
            for(Configuration* tc : e->targets){
                if(tc->assignment == UNKNOWN){
                    tc->assignment = ZERO;
                    C.push(tc);
                }
            }
        }
    }
}

void Global_FP_Algorithm::CalculateEdges(Configuration &v, EdgePicker &W){
    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Visisted;

    std::queue<Configuration*> C;
    Visisted.insert(&v);
    C.push(&v);

    while(!C.empty()){
        Configuration* c = C.front();
        C.pop();
        for(Edge* e : c->Successors){
            W.push(e);
            for(Configuration* tc : e->targets){
                auto result = Visisted.insert(tc);
                if(result.second)
                    C.push(tc);
            }
        }
    }
}

}//ctl
