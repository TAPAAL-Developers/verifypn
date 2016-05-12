#include "LocalFPA.h"
#include "../DependencyGraph/Configuration.h"
#include "../DependencyGraph/Edge.h"

#include <assert.h>
#include <iostream>

bool Algorithm::LocalFPA::search(DependencyGraph::BasicDependencyGraph &t_graph,
                                 SearchStrategy::iSequantialSearchStrategy &t_strategy)
{
    using namespace DependencyGraph;
    using TaskType = SearchStrategy::TaskType;

    graph = &t_graph;
    strategy = &t_strategy;

    std::cout << "Initial Configuration" << std::endl;
    Configuration *v = graph->initialConfiguration();
    explore(v);

    std::cout << "Exploring" << std::endl;
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
            if (c->assignment != DependencyGraph::ONE) {
                allOne = false;
                lastUndecided = c;
            }
        }
        //std::cout << "all one " << allOne << " has czero " << hasCZero << "last: " << lastUndecided << std::endl;

        if (e->is_negated) {
            //Process negation edge
            if(allOne) {}
            else if(!e->processed){
                addDependency(e, lastUndecided);
                if(lastUndecided->assignment == UNKNOWN){
                    explore(lastUndecided);
                }
            }
            else{
                finalAssign(e->source, ONE);
            }

        } else {
            //Process hyper edge
            if (allOne) {
                finalAssign(e->source, ONE);
            } else {
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

    std::cout << "Final Assignment " << v->assignmentToStr(v->assignment) << std::endl;
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
    assert(c->assignment == DependencyGraph::UNKNOWN);
    c->assignment = DependencyGraph::ZERO;
    graph->successors(c);

    for (DependencyGraph::Edge *succ : c->successors) {
        strategy->pushEdge(succ);
    }
}

void Algorithm::LocalFPA::addDependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}
