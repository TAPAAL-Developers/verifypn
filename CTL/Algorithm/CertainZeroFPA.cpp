#include "CertainZeroFPA.h"
#include "assert.h"
#include <iostream>

using namespace DependencyGraph;

//bool Algorithm::CertainZeroFPA::search(
//        DependencyGraph::BasicDependencyGraph &t_graph,
//        SearchStrategy::AbstractSearchStrategy &t_strategy
//) {
//    graph = &t_graph;
//    strategy = &t_strategy;
//    unsigned int currentDist = -1;

//    Configuration *v = graph->initialConfiguration();

//    explore(v);

//    Edge *e;
//    SearchStrategy::Message *m;

//    int r = strategy->pickTask(e, e, m, currentDist);

//    while (r != SearchStrategy::AbstractSearchStrategy::EMPTY) {

//        if(r == SearchStrategy::AbstractSearchStrategy::UNAVAILABLE){
//            currentDist = strategy->maxDistance();
//            r = strategy->pickTask(e, e, m, currentDist);
//        }
//        else if(r != SearchStrategy::AbstractSearchStrategy::NEGATIONEDGE){
//            currentDist = -1;
//        }

//        //std::cout << "process edge " << e << std::endl;
////        assert(r == 0 || r == 1);   //no messages

//        if (v->isDone()) {
//            break;
//        }

//        //if (e->source->isDone() || e->is_deleted) continue;

//        bool allOne = true;
//        bool hasCZero = false;
//        Configuration *lastUndecided = nullptr;

//        for (DependencyGraph::Configuration *c : e->targets) {
//            if (c->assignment == CZERO) {
//                hasCZero = true;
//            }
//            if (c->assignment != ONE) {
//                allOne = false;
//            }
//            if (!c->isDone()) {
//                lastUndecided = c;
//            }
//        }
//        //std::cout << "all one " << allOne << " has czero " << hasCZero << "last: " << lastUndecided << std::endl;

//        if (e->source->is_negated) {
//            //Process negation edge
//            if (allOne) {
//                e->source->removeSuccessor(e);
//                if (e->source->successors.empty()) {
//                    finalAssign(e->source, CZERO);
//                }
//            } else if (hasCZero) {
//                finalAssign(e->source, ONE);
//            } else {
//                assert(lastUndecided != nullptr);
//                if (lastUndecided->assignment == ZERO) {
//                    finalAssign(e->source, ONE);
//                } else {
//                    strategy->pushNegationEdge(e);
//                    lastUndecided->dependency_set.push_back(e);
//                    explore(lastUndecided);
//                }
//            }
//        } else {
//            //Process hyper edge
//            if (allOne) {
//                finalAssign(e->source, ONE);
//            } else if (hasCZero) {
//                e->source->removeSuccessor(e);
//                if (e->source->successors.empty()) {
//                    finalAssign(e->source, CZERO);
//                }
//            } else if (lastUndecided != nullptr) {
//                lastUndecided->dependency_set.push_back(e);
//                if (lastUndecided->assignment == UNKNOWN) {
//                    explore(lastUndecided);
//                }
//            }
//        }
//        r = strategy->pickTask(e, e, m, currentDist);
//    }

//    return (v->assignment == ONE) ? true : false;
//}

//void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a)
//{
//    assert(a == ONE || a == CZERO);

//    c->assignment = a;
//    for (DependencyGraph::Edge *e : c->dependency_set) {
//        if (e->source->is_negated) {
//            strategy->pushNegationEdge(e);
//        } else {
//            strategy->pushEdge(e);
//        }
//    }
//    c->dependency_set.clear();
//}

//void Algorithm::CertainZeroFPA::explore(Configuration *c)
//{
//    //std::cout << "Exploring " << c << std::endl;
//    //c->printConfiguration();
//    c->assignment = ZERO;
//    graph->successors(c);

//    if (c->successors.empty()) {
//        finalAssign(c, CZERO);
//    } else {
//        for (Edge *succ : c->successors) {
//            //std::cout << "push edge " << succ << std::endl;
//            if (succ->source->is_negated) {
//                strategy->pushNegationEdge(succ);
//            } else {
//                strategy->pushEdge(succ);
//            }
//        }
//    }
//}
