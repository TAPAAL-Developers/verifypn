#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/PetriNets/OnTheFlyDG.h"

#include <cassert>
#include <iostream>
#include "CTL/SearchStrategy/DFSSearch.h"

using namespace DependencyGraph;
using namespace SearchStrategy;

#ifndef NDEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x) if (!(x)) { std::cerr << "Assertion " << #x << " failed\n"; exit(1); }
#endif

void print_edge(Edge* e) {
#ifndef NDEBUG
    std::cerr << '(' << e->source->id;
    if (e->is_negated) {
        std::cerr << " -- ";
    } else {
        std::cerr << ", { ";
    }
    for (auto c: e->targets) {
        std::cerr << c->id << ' ';
    }
    std::cerr << (e->is_negated ? ")" : "})");
#endif
}

bool is_assignable(Edge* e) {
    auto c = e->source;
    if (c->nsuccs == 0) {
        return true;
    }
    bool allOne = true;
    for (auto v: e->targets) {
        if (v->assignment == CZERO)
            return c->nsuccs == 1;
        if (v->assignment != ONE)
            return false;
    }
    return true;
}

bool Algorithm::CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph& t_graph) {
    auto res = _search(t_graph);
    auto post_inv = [&](Edge* e) { return e->source->isDone() || test_invariant(e); };

    auto strat = std::dynamic_pointer_cast<DFSSearch>(strategy);
    if (!strat->empty()) {
        std::cerr << "W nonempty: |W| = " << strat->Wsize() << "\t|D| = " << strat->D.size() << "\t|N| = " << strat->N.size() << std::endl;
    }

    while (!strat->W.empty()) {
        auto e = strat->W.top();
        if (!post_inv(e)) {
            std::cerr << "Invariant failed after finishing algorithm";
        }
        strat->W.pop();
    }
    while (!strat->D.empty()) {
        auto e = strat->D.back();
        if (!post_inv(e)) {
            std::cerr << "Invariant failed after finishing algorithm";
        }
        strat->D.pop_back();
    }
    while (!strat->N.empty()) {
        auto e = strat->N.back();
        if (!post_inv(e)) {
            std::cerr << "Invariant failed after finishing algorithm";
        }
        strat->N.pop_back();
    }
    return res;
}

bool Algorithm::CertainZeroFPA::_search(DependencyGraph::BasicDependencyGraph& t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    explore(root);
    root->rank = 1;

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

void Algorithm::CertainZeroFPA::checkEdge(Edge* e, bool only_assign, bool was_dep) {
    if (e->handled) return;
#ifdef DG_SOURCE_CHECK
    ASSERT(test_invariant(e) || is_assignable(e));
    if (e->source->isDone()) {
        if (e->refcnt == 0) graph->release(e);
        return;
    }
#endif
    bool optim_happened = false;
#ifdef DG_LAZY_CHECK
    if (!only_assign /*&& !was_dep*/) {
#ifndef NDEBUG
        bool inv_good = test_invariant(e);
#endif
        bool allDone = e->source != root;
        for (auto* pre: e->source->dependency_set) {
            //if (preEdge->processed) {
            if (!pre->source->isDone()) {
                allDone = false;
                break;
            }
        }
        if (allDone) {
            e->source->passed = false;
            //if (!is_assignable(e))
            ++_optim_procs;
            optim_happened = true;
            //if(e->refcnt == 0) graph->release(e);
            return;
        }
#ifndef NDEBUG
        if (!inv_good && !was_dep) {
            std::cerr << "Failed invariant! At edge ";
            print_edge(e);
            std::cerr << '\n';
            std::cout << "	Configurations    : " << static_cast<PetriNets::OnTheFlyDG*>(graph)->configurationCount()
                      << "\n";
            std::cout << "	Markings          : " << static_cast<PetriNets::OnTheFlyDG*>(graph)->markingCount()
                      << "\n";
            std::cout << "	Edges             : " << _numberOfEdges << "\n";
            std::cout << "	Processed Edges   : " << _processedEdges << "\n";
            std::cout << "	Processed N. Edges: " << _processedNegationEdges << "\n";
            std::cout << "	Explored Configs  : " << _exploredConfigurations << "\n";
            std::cout << "	Dependent Clears  : " << _optim_procs << "\n";
            throw base_error("Fatal: Invariant inv_good failed!\n");
        }
        //assert(inv_good);
#endif
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

#ifdef NDEBUG__
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
    Configuration* lastUndecided = nullptr;
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
                    assert(!optim_happened);
                    lastUndecided = *it;
                }
            }
            pit = it;
            ++it;
        }
    }
#ifdef NDEBUG__
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
#ifdef NDEBUG__
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
                    //if (lastUndecided->assignment == UNKNOWN) {
                    assert(!optim_happened);
                    explore(lastUndecided);
                    lastUndecided->rank = e->source->rank + 1;
                }
            }
        }
    } else { // not negated
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
                assert(!optim_happened);
                explore(lastUndecided);
            }
        }
    }
    if (e->refcnt > 0 && !only_assign) e->processed = true;
    if (e->refcnt == 0) graph->release(e);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Edge* e, DependencyGraph::Assignment a) {
#ifndef NDEBUG
    //std::cerr << "assigning to "; print_edge(e); std::cerr << std::endl;
#endif
    finalAssign(e->source, a);
    backprop(e->source);
}

void Algorithm::CertainZeroFPA::finalAssign(DependencyGraph::Configuration* c, DependencyGraph::Assignment a) {
    assert(a == ONE || a == CZERO);

    c->assignment = a;
    c->nsuccs = 0;
    for (DependencyGraph::Edge* e: c->dependency_set) {
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
                //strategy->pushDependency(e);
            } else {
                //strategy->pushNegation(e);
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

void Algorithm::CertainZeroFPA::explore(Configuration* c) {
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
                for (Edge* e: succs) {
                    assert(e->refcnt <= 1);
                    if (e->refcnt >= 1) --e->refcnt;
                    if (e->refcnt == 0) graph->release(e);
                }
                return;
            }
        }

        if (c->nsuccs == 0) {
            for (Edge* e: succs) {
                assert(e->refcnt <= 1);
                if (e->refcnt >= 1) --e->refcnt;
                if (e->refcnt == 0) graph->release(e);
            }
            finalAssign(c, CZERO);
            return;
        }

        for (Edge* succ: succs) {
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


std::pair<Configuration *, Assignment> Algorithm::CertainZeroFPA::eval_edge(DependencyGraph::Edge *e) {
    bool allOne = true, hasCZero = false;
    Configuration *retval = nullptr;
    Assignment a = ZERO;
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
                break;
            } else if (retval == nullptr) {
                retval = *it;
            }
        }
        pit = it;
        ++it;
    }

    if (!e->is_negated) {
        if (hasCZero) {
            --e->source->nsuccs;
            e->handled = true;
            if (e->source->nsuccs == 0) {
                e->source->assignment = a = CZERO;
            }
        } else if (allOne) {
            e->source->assignment = a = ONE;
        }
    } else { // is_negated
        if (hasCZero) {
            e->source->assignment = a = ONE;
        } else if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            if (e->source->nsuccs == 0) {
                a = CZERO;
            }
        } else if (e->processed && retval->assignment == ZERO) {
            e->source->assignment = a = ONE;
        }
    }
    return std::make_pair(retval, a);
}

void Algorithm::CertainZeroFPA::backprop(Configuration* conf) {
    assert(conf->isDone());
    std::unordered_set<Configuration*> W;
    for (auto e : conf->dependency_set) {
        W.insert(e->source);
    }
    W.insert(conf);
    while (!W.empty()) {
        auto vit = W.begin();
        auto v = *vit;
        if (v->isDone()) {
            W.erase(vit);
            continue;
        }
        assert(v->rank != std::numeric_limits<size_t>::max());
        //assert(v == root || dependent_search(v, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
        //assert(v->isDone() || v->instack); // bad assert
        auto pit = v->dependency_set.before_begin();
        auto it = v->dependency_set.begin();
        while (it != v->dependency_set.end()) {
            auto& e = *it;
            if (!e->source->isDone()) {
                ((e->is_negated) ? _processedNegationEdges : _processedEdges) += 1;
                auto [_, a] = eval_edge(e);
                if (a == ONE || a == CZERO) {
                    // backpropagated assignment; keep going!
                    W.insert(e->source);
                    strategy->pushDependency(e);
                    //v->dependency_set.erase_after(pit);
                    it = pit;
                }
                if (e->source->rank > conf->rank && e->source->passed) {
                    W.insert(e->source);
                    e->source->passed = false;
                }
            }
            pit = it;
            ++it;
        }
        v->dependency_set.clear();
        W.erase(vit);
        /*continue;
        for (auto e : v->dependency_set) {
            if (!e->source->isDone()) {
                eval_edge(e);
                if (e->source->isDone()) {
                    // backpropagated assignment; keep going!
                    W.push_back(e->source);
                }
                else {
                    e->source->resucs.push_back(e);
                }
            }
        }
        v->dependency_set.clear();*/
    }
}

