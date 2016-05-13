#include "CertainZeroFPA.h"
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <iostream>

using namespace DependencyGraph;

bool Algorithm::CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph &t_graph,
        SearchStrategy::iSequantialSearchStrategy &t_strategy)
{
//    std::cout << "Instantiating" << std::endl;

    using namespace SearchStrategy;
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



        if (v->isDone()) {
            break;
        }

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
        //std::cout << "all one " << allOne << " has czero " << hasCZero << "last: " << lastUndecided << std::endl;

        if (e->is_negated) {
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
                if (lastUndecided->assignment == ZERO && e->processed) {
                    finalAssign(e->source, ONE);
                } else {
                    addDependency(e, lastUndecided);
                    if (lastUndecided->assignment == UNKNOWN) {
                        explore(lastUndecided);
                    }
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
                addDependency(e, lastUndecided);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
        e->processed = true;
//        std::cout << "Picking task" << std::endl;
        r = strategy->pickTask(e);
//        std::cout << "Picked" << std::endl;
    }

//    std::cout << "Final Assignment " << std::boolalpha << (v->assignment == ONE ? true : false) << std::endl;
    return (v->assignment == ONE) ? true : false;
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
{
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    for (DependencyGraph::Edge *e : c->dependency_set) {
        strategy->pushDependency(e);
    }
    c->dependency_set.clear();
}

void Algorithm::CertainZeroFPA::explore(Configuration *c)
{
//    std::cout << "Exploring " << c << std::endl;
    //c->printConfiguration();
    c->assignment = ZERO;
    graph->successors(c);

    if (c->successors.empty()) {
        finalAssign(c, CZERO);
    }
    else {
        for (Edge *succ : c->successors) {
//            std::cout << "push edge " << succ << std::endl;
            strategy->pushEdge(succ);
        }
    }
}

void Algorithm::CertainZeroFPA::addDependency(Edge *e, Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}
