#include "LocalFPA.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"

#include <assert.h>

bool Algorithm::LocalFPA::search(DependencyGraph::BasicDependencyGraph &t_graph,
                                 SearchStrategy::iSequantialSearchStrategy &t_strategy)
{
    using namespace DependencyGraph;
    using TaskType = SearchStrategy::TaskType;

    graph = &t_graph;
    strategy = &t_strategy;

//    std::cout << "Initial Configuration" << std::endl;
    Configuration *v = graph->initialConfiguration();
    explore(v);

//    std::cout << "Exploring" << std::endl;
    Edge *e;
    int r = strategy->pickTask(e);

//    v->printConfiguration();

    while (r != TaskType::EMPTY) {

//        std::cout << std::endl;
//        e->source->printConfiguration();
//        for (Configuration *c : e->targets) {
//            c->printConfiguration();
//        }
//        std::cout << std::endl;

        if (v->assignment == DependencyGraph::ONE) {
            break;
        }

        bool allOne = true;
        Configuration *lastUndecided = nullptr;

        for (DependencyGraph::Configuration *c : e->targets) {
            if (c->assignment != ONE) {
                allOne = false;
                lastUndecided = c;
            }
        }
        //std::cout << "all one " << allOne << " has czero " << hasCZero << "last: " << lastUndecided << std::endl;

        if (e->is_negated) {
            //Process negation edge
            if (allOne) {
                e->source->removeSuccessor(e);
                if (e->source->successors.empty()) {
                    finalAssign(e->source, CZERO);
                }
            } else {
                assert(lastUndecided != nullptr);
                if (lastUndecided->assignment == ZERO) {
                    finalAssign(e->source, ONE);
                } else {
                    strategy->pushEdge(e);
                    addDependency(e, lastUndecided);
                    explore(lastUndecided);
                }
            }
        } else {
            //Process hyper edge
            if (allOne) {
                finalAssign(e->source, ONE);
            } else if (lastUndecided != nullptr) {
                addDependency(e, lastUndecided);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
//        std::cout << "Picking task" << std::endl;
        r = strategy->pickTask(e);
//        std::cout << "Picked" << std::endl;
    }

//    std::cout << "Final Assignment " << std::boolalpha << (v->assignment == ONE ? true : false) << std::endl;
    return (v->assignment == ONE) ? true : false;
}

void Algorithm::LocalFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == DependencyGraph::ONE);
    c->assignment = a;

    for(DependencyGraph::Edge *e : c->dependency_set){
        strategy->pushEdge(e);
    }

    c->dependency_set.clear();
}

void Algorithm::LocalFPA::explore(DependencyGraph::Configuration *c)
{
    c->assignment = DependencyGraph::ZERO;
    graph->successors(c);

    for (DependencyGraph::Edge *succ : c->successors) {
//            std::cout << "push edge " << succ << std::endl;
        strategy->pushEdge(succ);
//            if (succ->source->is_negated) {
//                strategy->pushEdge(succ);
//            } else {
//                strategy->pushEdge(succ);
//            }
    }
}
