#include "LocalFPA.h"

bool ctl::LocalFPA::search(ctl::DependencyGraph &t_graph, ctl::AbstractSearchStrategy &W)
{
    PriorityQueue N;

    Configuration &v = t_graph.initialConfiguration();
    v.assignment = ZERO;

    for(Edge *e : t_graph.successors(v)){
        W.push(e);
        if(e->source->IsNegated)
            N.push(e);
    }

    while(!W.empty() || !N.empty()){
        Edge *e;

        if(!W.empty()) {
            e = W.pop();
            //std::cout << "Popped negation edge from N: \n" << std::flush;
           // e->edgePrinter();
        }
        else if (!N.empty()) {
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

                    for(Edge *succ : t_graph.successors(*tc)){
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
