#include "CertainZeroFPA.h"

namespace ctl{

bool CertainZeroFPA::search(DependencyGraph &t_graph, AbstractSearchStrategy &W)
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
        }
        else if (!N.empty()) {
            e = N.top();
            N.pop();
        }

//        e->edgePrinter();

        /*****************************************************************/
        /*Data handling*/
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        Configuration * lastUnknown = NULL;
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
                lastUnknown = c;
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
                    e->source->assignment = CZERO;
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
        else if(lastUnknown != NULL){
            Configuration *tc = lastUnknown;
            tc->assignment = ZERO;
            tc->DependencySet.push_back(e);
            t_graph.successors(*tc);

            if(tc->Successors.empty()){
                tc->assignment = CZERO;
               W.push_dependency(e);
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
        e->processed = true;
    }

//    std::cout << "Final Assignment: " << v.assignment << " " << ((v.assignment == ONE) ? true : false) << std::endl;
    return (v.assignment == ONE) ? true : false;
}

}
