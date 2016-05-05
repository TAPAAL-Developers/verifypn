#include "CertainZeroFPA.h"
#include "assert.h"

using namespace DependencyGraph;

bool Algorithm::CertainZeroFPA::search(
        DependencyGraph::BasicDependencyGraph &t_graph,
        SearchStrategy::AbstractSearchStrategy &t_strategy
) {
/*
    graph = &t_graph;
    strategy = &t_strategy;

    Configuration *v = graph->initialConfiguration();
    explore(v);

    while (!strategy->done()) {
        if (v->isDone()) {
            break;
        }

        Edge *e = strategy->pickTask();

        if (e->source->isDone() || e->is_deleted) continue;

        bool allOne = true;
        bool hasCZero = false;
        Configuration *lastUndecided = nullptr;

        for (DependencyGraph::Configuration *c : e->targets) {
            if (c->assignment == CZERO) {
                hasCZero = true;
            }
            if (c->assignment != ONE) {
                allOne = false;
            }
            if (!c->isDone()) {
                lastUndecided = c;
            }
        }

        if (e->source->is_negated) {
            //Process negation edge
            if (allOne) {
                e->source->removeSuccessor(e);
                if (e->source->successors.empty()) {
                    finalAssign(e->source, CZERO);
                }
            } else if (hasCZero) {
                finalAssign(e->source, ONE);
            } else {
                assert(lastUndecided != nullptr);
                if (lastUndecided->assignment == ZERO) {
                    finalAssign(e->source, ONE);
                } else {
                    strategy->push(e);
                    lastUndecided->dependency_set.push_back(e);
                    explore(lastUndecided);
                }
            }
        } else {
            //Process hyper edge
            if (allOne) {
                finalAssign(e->source, ONE);
            } else if (hasCZero) {
                e->source->removeSuccessor(e);
                if (e->source->successors.empty()) {
                    finalAssign(e->source, CZERO);
                }
            } else if (lastUndecided != nullptr) {
                lastUndecided->dependency_set.push_back(e);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
    }
*/
    return false;//(v->assignment == ONE) ? true : false;
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    /*assert(a == ONE || a == CZERO);

    c->assignment = a;
    for (DependencyGraph::Edge *e : c->dependency_set) {
        strategy->push(e);
    }
    c->dependency_set.clear();*/
}

void Algorithm::CertainZeroFPA::explore(Configuration *c)
{
   /* c->assignment = ZERO;
    graph->successors(*c);

    if (c->successors.empty()) {
        finalAssign(c, CZERO);
    } else {
        for (Edge *succ : c->successors) {
            strategy->push(succ);
        }
    }*/
}
