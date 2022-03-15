#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/PetriNets/OnTheFlyDG.h"

#include <cassert>
#include <iostream>

using namespace DependencyGraph;
using namespace SearchStrategy;

void print_edge(Edge *e)
{
#ifndef NDEBUG
    std::cerr << '(' << e->source->id;
    if (e->is_negated) {
        std::cerr << " -- ";
    }
    else {
        std::cerr << ", { ";
    }
    for (auto c : e->targets) {
        std::cerr << c->id << ' ';
    }
    std::cerr << (e->is_negated ? ")" : "})");
#endif
}

bool Algorithm::CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph &t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    {
        explore(root);
    }
#ifdef DG_REFCOUNTING
    root->refc = 1;
#endif

    size_t cnt = 0;
    while (!strategy->empty()) {
        while (true) {
            auto[e, was_dep] = strategy->popEdge(false);
            if (!e) break;

            ++e->refcnt;
            assert(e->refcnt >= 1);
            checkEdge(e, false, was_dep);
            assert(e->refcnt >= -1);
            if (e->refcnt > 0) --e->refcnt;
            if (e->refcnt == 0) graph->release(e);
            ++cnt;
            if ((cnt % 1000) == 0) strategy->trivialNegation();
            if (root->isDone()) return root->assignment == ONE;
        }

        if (root->isDone()) return root->assignment == ONE;

        if (!strategy->trivialNegation()) {
            cnt = 0;
            strategy->releaseNegationEdges(strategy->maxDistance());
            continue;
        }
    }

    return root->assignment == ONE;
}

void Algorithm::CertainZeroFPA::checkEdge(Edge *e, bool only_assign, bool was_dep) {
    if (e->handled) return;
#ifdef DG_SOURCE_CHECK
    assert(test_invariant(e));
    if (e->source->isDone()) {
        if (e->refcnt == 0) graph->release(e);
        return;
    }
#endif
#ifdef DG_LAZY_CHECK
    if (!only_assign && !was_dep) {
//#ifndef NDEBUG
        bool inv_good = false;
        if (e->source != root) {
            std::stack<DependencyGraph::Edge*> W;
            std::unordered_set<DependencyGraph::Edge*> passed;

            W.push(e);
            while (!W.empty()) {
                auto edge = W.top(); W.pop();
                if (std::find(std::begin(passed), std::end(passed), edge) == std::end(passed)) {
                    if (edge->source->isDone() && edge != e) {
                        continue;
                    }
                    else {
                        for (auto d : edge->source->dependency_set) {
                            if (d->source == root) {
                                inv_good = true; break;
                            }
                            if (!d->source->isDone()) {
                                W.push(d);
                            }
                        }
                        passed.insert(edge);
                    }
                }
            }
        }
        else inv_good = true;
//#endif
        bool allDone = e->source != root;
        for (auto *pre: e->source->dependency_set) {
            //if (preEdge->processed) {
            if (!pre->source->isDone()) {
                allDone = false;
                break;
            }
        }
        if (allDone) {
            e->source->passed = false;
            //if(e->refcnt == 0) graph->release(e);
            return;
        }
        if (!inv_good) {
            std::cerr << "Failed invariant! At edge "; print_edge(e); std::cerr << '\n';
            std::cout << "	Configurations    : " << static_cast<PetriNets::OnTheFlyDG*>(graph)->configurationCount() << "\n";
            std::cout << "	Markings          : " << static_cast<PetriNets::OnTheFlyDG*>(graph)->markingCount() << "\n";
            std::cout << "	Edges             : " << _numberOfEdges << "\n";
            std::cout << "	Processed Edges   : " << _processedEdges << "\n";
            std::cout << "	Processed N. Edges: " << _processedNegationEdges << "\n";
            std::cout << "	Explored Configs  : " << _exploredConfigurations << "\n";
                throw base_error("Fatal: Invariant inv_good failed!\n");
        }
        assert(inv_good);
    }
#endif //defined(DG_LAZY_CHECK)
#ifdef DG_REFCOUNTING
    if (!only_assign && !was_dep && e->source->refc == 0) {
        assert(e->source != root);
        assert(e->source->dependency_set.empty());
        if (!e->source->isDone())
            e->source->passed = false;
        return;
    }
#endif

#ifndef NDEBUG
    if (!only_assign) {
        std::cerr << "checking ";
        if (was_dep) {
            std::cerr << "dependency ";
        }
        else {
            std::cerr << "successor  ";
        }
        print_edge(e);
    }
#endif
    bool allOne = true;
    bool hasCZero = false;
    //auto pre_empty = e->targets.empty();
    Configuration *lastUndecided = nullptr;
    {
        auto it = e->targets.begin();
        auto pit = e->targets.before_begin();
        while (it != e->targets.end()) {
            if ((*it)->assignment == ONE) {
                e->targets.erase_after(pit);
                it = pit;
            } else {
                allOne = false;
                if ((*it)->assignment == CZERO) {
                    hasCZero = true;
                    //assert(e->assignment == CZERO || only_assign);
                    break;
                } else if (lastUndecided == nullptr) {
                    lastUndecided = *it;
                }
            }
            pit = it;
            ++it;
        }
    }
#ifndef NDEBUG
    if (!only_assign) {
        if (lastUndecided != nullptr) {
            std::cerr << " -> " << lastUndecided->id << std::endl;
        }
        else {
            assert(allOne || hasCZero);
            std::cerr << " --- assigning value!\n";
        }
    }
#endif
    /*if(e->targets.empty())
    {
        assert(e->assignment == ONE || e->children == 0);
    }*/

    if (e->is_negated) {
        _processedNegationEdges += 1;
        //Process negation edge
        if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if (only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e, CZERO);
            }
            if (e->refcnt == 0) { graph->release(e); }
        } else if (hasCZero) {
            finalAssign(e, ONE);
        } else {
            assert(lastUndecided != nullptr);
            if (only_assign) return;
            if (lastUndecided->assignment == ZERO && e->processed) {
#ifndef NDEBUG
                std::cerr << " --- assigning value!\n";
#endif
                finalAssign(e, ONE);
            } else {
                if (!e->processed) {
                    strategy->pushNegation(e);
                }
#ifdef DG_REFCOUNTING
                ++lastUndecided->refc;
#endif
                lastUndecided->addDependency(e);
                if (!lastUndecided->passed) {
                    explore(lastUndecided);
                }
//                if (lastUndecided->assignment == UNKNOWN) {
//                    explore(lastUndecided);
//                }
            }
        }
    } else {
        _processedEdges += 1;
        //Process hyper edge
        if (allOne) {
            finalAssign(e, ONE);
        } else if (hasCZero) {
            --e->source->nsuccs;
            e->handled = true;
            assert(e->refcnt > 0);
            if (only_assign) --e->refcnt;
            if (e->source->nsuccs == 0) {
                finalAssign(e, CZERO);
            }
            if (e->refcnt == 0) { graph->release(e); }

        } else if (lastUndecided != nullptr) {
            if (only_assign) return;
            if (!e->processed) {
                if (!lastUndecided->isDone()) {
                    for (auto t: e->targets) {
#ifdef DG_REFCOUNTING
                        ++t->refc;
#endif
                        t->addDependency(e);
                    }
                }
            }
            //if (lastUndecided->assignment == UNKNOWN) {
            if (!lastUndecided->passed) {
                explore(lastUndecided);
            }
        }
    }
    if (e->refcnt > 0 && !only_assign) e->processed = true;
    if (e->refcnt == 0) graph->release(e);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Edge *e, DependencyGraph::Assignment a) {
#ifndef NDEBUG
    //std::cerr << "assigning to "; print_edge(e); std::cerr << std::endl;
#endif
    finalAssign(e->source, a);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a) {
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    c->nsuccs = 0;
    for (DependencyGraph::Edge *e: c->dependency_set) {
        if (!e->source->isDone()) {
            if (a == CZERO) {
                /*e->assignment = CZERO;*/
            } else if (a == ONE) {
                /*assert(e->children >= 1);
                --e->children;
                if(e->children == 0)
                    e->assignment = ONE;*/
            }
            if (!e->is_negated || a == CZERO) {
                strategy->pushDependency(e);
            } else {
                strategy->pushNegation(e);
            }
        }
        assert(e->refcnt > 0);
        --e->refcnt;
        if (e->refcnt == 0) graph->release(e);
    }
#ifdef DG_REFCOUNTING
    for (auto v: c->forward_dependency_set) {
        assert(v->refc > 0);
        //assert(!v->dependency_set.empty());
        v->remove_dependent(c);
    }
    c->forward_dependency_set.clear();
#endif

    c->dependency_set.clear();
}

void Algorithm::CertainZeroFPA::explore(Configuration *c) {
    c->assignment = ZERO;
    c->passed = true;

    {
        auto succs = graph->successors(c);
        c->nsuccs = succs.size();

        _exploredConfigurations += 1;
        _numberOfEdges += c->nsuccs;
        // before we start exploring, lets check if any of them determine 
        // the outcome already!

        for (int32_t i = c->nsuccs - 1; i >= 0; --i) {
            checkEdge(succs[i], true);
            if (c->isDone()) {
                for (Edge *e: succs) {
                    assert(e->refcnt <= 1);
                    if (e->refcnt >= 1) --e->refcnt;
                    if (e->refcnt == 0) graph->release(e);
                }
                return;
            }
        }

        if (c->nsuccs == 0) {
            for (Edge *e: succs) {
                assert(e->refcnt <= 1);
                if (e->refcnt >= 1) --e->refcnt;
                if (e->refcnt == 0) graph->release(e);
            }
            finalAssign(c, CZERO);
            return;
        }

        for (Edge *succ: succs) {
            assert(succ->refcnt <= 1);
            if (succ->refcnt > 0) {
                strategy->pushEdge(succ);
                --succ->refcnt;
                if (succ->refcnt == 0) graph->release(succ);
            } else if (succ->refcnt == 0) {
                graph->release(succ);
            }
        }
    }
    strategy->flush();
}
