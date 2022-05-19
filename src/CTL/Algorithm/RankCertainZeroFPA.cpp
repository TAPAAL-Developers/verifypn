#include "CTL/Algorithm/RankCertainZeroFPA.h"
#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/DependencyGraph/Configuration.h"

#include <cassert>
#include <iostream>
#include "CTL/SearchStrategy/DFSSearch.h"

using namespace DependencyGraph;
using namespace SearchStrategy;

#ifndef NDEBUG
#define ASSERT(x) assert(x)
#define DEBUG_ONLY(x) x
#else
#define ASSERT(x) if (!(x)) { std::cerr << "Assertion " << #x << " failed\n"; exit(1); }
#define DEBUG_ONLY(x)
#endif

DEBUG_ONLY(void print_edge(Edge* e) {
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
})

void set_assignment(Configuration *c, Assignment a) {
    c->assignment = a;
    //DEBUG_ONLY(std::cerr << "ASSIGN: [" << c->id << "]: " << to_string(static_cast<Assignment>(c->assignment)) << "\n";)
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

bool Algorithm::RankCertainZeroFPA::search(DependencyGraph::BasicDependencyGraph& t_graph) {
    auto res = _search(t_graph);
    /*auto post_inv = [&](Edge* e) { return e->source->isDone() || test_invariant(e); };

    auto strat = std::dynamic_pointer_cast<DFSSearch>(strategy);
    if (!strat->empty()) {
        DEBUG_ONLY(std::cerr << "W nonempty: |W| = " << strat->Wsize() << "\t|D| = " << strat->D.size() << "\t|N| = " << strat->N.size() << std::endl;)
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
    }*/
    return res;
}

bool Algorithm::RankCertainZeroFPA::_search(DependencyGraph::BasicDependencyGraph& t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    root->rank = 1;
    std::stack<std::pair<DependencyGraph::Configuration*, std::vector<DependencyGraph::Edge*>>> waiting;
    waiting.emplace(root, graph->successors(root));
    root->assignment = ZERO;
    //std::cerr << "PUSH [" << root->id << "]" << std::endl;
#ifdef DG_REFCOUNTING
    root->refc = 1;
#endif

    auto do_pop = [&waiting,this]() {
        auto& [conf, edges] = waiting.top();
        for(auto* e : edges)
        {
            e->status = DependencyGraph::EdgeStatus::NotWaiting;
            e->refcnt = 0;
            graph->release(e);
        }
        waiting.pop();
    };

    while(!waiting.empty() && !root->isDone())
    {
        auto& [conf, edges] = waiting.top();
        if(conf->isDone()) {
            backprop(conf);
            //std::cerr << "POP [" << conf->id << "] (assign(104) = " << to_string((Assignment)conf->assignment) << std::endl;
            do_pop();
            continue;
        }
        Configuration* undecided = nullptr;
        Edge* undecided_edge = nullptr;
        bool all_czero = true;
        for(auto& e : edges)
        {
            auto [ud, val] = eval_edge(e);
            //print_edge(e);
            //std::cerr << to_string(val);
            //std::cerr << std::endl;
            if((val == ONE && !e->is_negated) ||
               (val == CZERO && e->is_negated))
            {
                set_assignment(conf, ONE);
                all_czero = false;
                break;
            }
            else if((val == CZERO && !e->is_negated) ||
                     (val == ONE && e->is_negated))
            {
                // skip
            }
            else if(ud != nullptr && (
                    undecided == nullptr ||
                        (undecided->assignment == ZERO && ud->assignment == UNKNOWN)))
            {
                all_czero = false;
                undecided = ud;
                undecided_edge = e;
            }
            if(conf->isDone())
                break;
        }

        if(all_czero)
            set_assignment(conf, CZERO);

        if(conf->isDone()) {
            backprop(conf);
            //std::cerr << "POP [" << conf->id << "] (assign(124) = " << to_string((Assignment)conf->assignment) << std::endl;
            do_pop();
            continue;
        }

        if(undecided == nullptr || undecided->assignment != UNKNOWN) {
            //std::cerr << "POP [" << conf->id << "] (undecided == nullptr)" << std::endl;
            do_pop();
            continue;
        }
        else
        {
            //std::cerr << "PUSH [" << undecided->id << "]" << std::endl;
            undecided->addDependency(undecided_edge);
            undecided->rank = conf->rank + 1;
            undecided->assignment = ZERO;
            waiting.emplace(undecided, graph->successors(undecided));
        }
    }

    return root->assignment == ONE;
}

void Algorithm::RankCertainZeroFPA::checkEdge(Edge* e, bool only_assign, bool was_dep) {
    if (e->handled) return;
#ifdef DG_SOURCE_CHECK
    //ASSERT(test_invariant(e) || is_assignable(e));
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


    /*DEBUG_ONLY(if (!only_assign) {
        std::cerr << "checking ";
        if (was_dep) {
            std::cerr << "dependency ";
        }
        else {
            std::cerr << "successor  ";
        }
        print_edge(e); std::cerr << '\n';
    })*/
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
                else if(lastUndecided != nullptr && lastUndecided->assignment == UNKNOWN && (*it)->assignment == ZERO)
                {
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
                    lastUndecided->rank = e->source->rank + 1;
                    explore(lastUndecided);
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
                lastUndecided->rank = e->source->rank + 1;
                explore(lastUndecided);
            }
        }
    }
    if (e->refcnt > 0 && !only_assign) e->processed = true;
    if (e->refcnt == 0) graph->release(e);
}

void Algorithm::RankCertainZeroFPA::finalAssign(DependencyGraph::Edge* e, DependencyGraph::Assignment a) {
#ifndef NDEBUG
    //std::cerr << "assigning to "; print_edge(e); std::cerr << std::endl;
#endif
    //finalAssign(e->source, a);
    set_assignment(e->source, a);
    backprop(e->source);
}

void Algorithm::RankCertainZeroFPA::finalAssign(DependencyGraph::Configuration* c, DependencyGraph::Assignment a) {
    assert(a == ONE || a == CZERO);

    set_assignment(c, a);
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
    //c->dependency_set.clear();
}

std::vector<Edge*> Algorithm::RankCertainZeroFPA::explore(Configuration* c) {
    set_assignment(c, ZERO);
    assert(!c->passed);
    c->passed = true;
    //DEBUG_ONLY(std::cerr << "ASSIGN: [" << c->id << "]: " << to_string(static_cast<Assignment>(c->assignment)) << "\n";)

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
            return {};
        }
    }

    if (c->nsuccs == 0) {
        for (Edge* e : succs) {
            assert(e->refcnt <= 1);
            if (e->refcnt >= 1) --e->refcnt;
            if (e->refcnt == 0) graph->release(e);
        }
        finalAssign(c, CZERO);
        return {};
    }
    else
    {
        std::vector<Edge*> edges;
        for (Edge* succ: succs) {
            assert(succ->refcnt <= 1);
            if (succ->refcnt > 0) {
                if(succ->status != DependencyGraph::EdgeStatus::NotWaiting || succ->source->isDone())
                    --succ->refcnt;
                else
                {
                    succ->status = DependencyGraph::EdgeStatus::InWaiting;
                    edges.emplace_back(succ);
                }
                if (succ->refcnt == 0)
                    graph->release(succ);
            } else if (succ->refcnt == 0) {
                graph->release(succ);
            }
        }
        return edges;
    }
}


std::pair<Configuration *, Assignment> Algorithm::RankCertainZeroFPA::eval_edge(DependencyGraph::Edge *e) {
    bool allOne = true, hasCZero = false;
    Configuration *retval = nullptr;
    Assignment a = ZERO;
    /*DEBUG_ONLY(
        std::cerr << "EVAL [";
        print_edge(e);
        std::cerr << "]\n";
    )*/
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
            } else if (retval == nullptr || (retval->assignment == UNKNOWN && (*it)->assignment == ZERO)) {
                retval = *it;
            }
        }
        pit = it;
        ++it;
    }

    if(allOne)
        return std::make_pair(nullptr, ONE);
    if(hasCZero)
        return std::make_pair(nullptr, CZERO);
    if(retval)
        return std::make_pair(retval, (Assignment)retval->assignment);
    else
    {
        assert(false);
        return std::make_pair(retval, ZERO);
    }
}

void Algorithm::RankCertainZeroFPA::backprop_edge(Edge* edge) {
    /*std::unordered_set<Edge*> W;
    for (auto d : edge->source->dependency_set) {
        W.insert(d);
    }

    auto push_dep = [&](Edge* e) {
        for (auto* dep: e->source->dependency_set) {
            W.insert(dep);
        }
    };

    while (!W.empty()) {
        auto eit = W.begin(); auto e = *eit;
        auto v = e->source;
        if (v->isDone()) {
            W.erase(eit);
            continue;
        }
        auto [c, a] = eval_edge(e);
        if (a == ONE || a == CZERO) {
            push_dep(e);
            strategy->pushDependency(e);
        }
        if (e->source->rank > edge->source->rank && e->source->passed) {
            push_dep(e);
            e->source->passed = false;
        }

        e->source->dependency_set.clear();
        W.erase(eit);
    }*/
    assert(edge->source->isDone());
}

void Algorithm::RankCertainZeroFPA::backprop(Configuration* source) {
    assert(source->isDone());
    std::stack<Configuration*> waiting;
    waiting.emplace(source);

    while (!waiting.empty()) {
        auto* conf = waiting.top();
        //assert(conf->isDone() || conf->passed == false);

        waiting.pop();
        auto prev = conf->dependency_set.before_begin();
        auto cur = conf->dependency_set.begin();
        while(cur != conf->dependency_set.end())
        {
            auto* e = *cur;
            auto* c = e->source;
            if (!c->isDone())
            {
                eval_edge(e);
                if (c->isDone())
                {
                    // was not done, but is now.
                    waiting.emplace(c);
                }
                else
                {
                    if(c->rank < source->rank)
                    {

                    }
                    else if(c->rank == source->rank)
                    {
                        assert(c == source);
                        assert(false);
                    }
                    else // c->rank > source->rank
                    {
                        if(c->passed)
                        {
                            c->passed = false;
                            waiting.emplace(c);
                        }
                    }
                }
            }
            conf->dependency_set.erase_after(prev);
            cur = prev;
            ++cur;
        }
    }
}
