/* Copyright (C) 2022  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "CTL/Algorithm/CZCycleDetectFPA.h"
#include "CTL/PetriNets/PetriConfig.h"

using namespace DependencyGraph;

bool Algorithm::CZCycleDetectFPA::search(BasicDependencyGraph &graph) {
    this->graph = &graph;
    this->root = graph.initialConfiguration();

    push_edge(root);
    root->rank = 0;

    // TODO stats
    while (!_dstack.empty()) {
        auto c = _dstack.top();
        if (c->isDone() || (c->sucs.empty() && c->resucs.empty())) {
            c->sucs.clear();
            c->instack = false;
            _dstack.pop();
            continue;
        }
#ifndef NDEBUG
        assert(c == root || dependent_search(c, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
        assert(c == root || dependent_search(c, [&](auto c) { return c == root; }, [] (Configuration* c) { return c->isDone(); }));
#endif

        auto e = pick_edge(c); //c->sucs.front(); c->sucs.pop();
        if (e == nullptr) continue;
        if (e->handled)
            continue;
        assert(e->source == c);


        auto [next, _] = eval_edge(e);
        ((e->is_negated) ? _processedNegationEdges : _processedEdges) += 1;
        //assert(next != nullptr || c->isDone());

        if (c->isDone()) {
            backprop(c);
        } else if (next != nullptr && !next->isDone()) {
            next->addDependency(e);
            if (next->assignment == UNKNOWN || next->recheck) {
                assert(next->rank == std::numeric_limits<uint64_t>::max() || next->recheck);
                push_edge(next);
                next->recheck = false;
                if (next->nsuccs == 1 && c->nsuccs == 1) {
                    next->rank = c->rank;
                } else {
                    next->rank = c->rank + 1;
                }
            } else {
                if (next->instack && next->rank == c->rank) {
                    if (next == c && c->nsuccs > 1) {
                        --c->nsuccs;
                    }
                    else {
                        assert(c->nsuccs <= 1 && next->nsuccs <= 1);
                        assign_value(c, CZERO);
                        backprop(c);
                    }
                }
            }
            if (e->is_negated && !e->processed) {
                c->resucs.push_back(e);
            }
        }
        if (root->isDone()) {
            return root->assignment == ONE;
        }
        e->processed = true;
    }
    return root->assignment == ONE;
}

Edge *Algorithm::CZCycleDetectFPA::pick_edge(DependencyGraph::Configuration *conf) {
    if (conf->sucs.empty()) {
        assert(!conf->resucs.empty());
        auto e = conf->resucs.back();
        conf->resucs.pop_back();
        return e;
    } else {
        auto e = conf->sucs.front();
        conf->sucs.pop();
        return e;
    }
}

void Algorithm::CZCycleDetectFPA::push_edge(Configuration *conf) {
    // try partial case for suc conf with assign ? but non-empty resucs.
    assert(conf->assignment == UNKNOWN || conf->recheck);
    assert(!conf->instack);
    conf->assignment = ZERO;
    auto sucs = graph->successors(conf);
    conf->nsuccs = sucs.size();
    ++_exploredConfigurations;
    _numberOfEdges += conf->nsuccs;
    assert(sucs.size() <= std::numeric_limits<uint32_t>::max());

    for (int32_t i = conf->nsuccs - 1; i >= 0; --i) {
        eval_edge(sucs[i]);
        if (conf->isDone()) {
            /*for (Edge *e: sucs) {
                assert(e->refcnt <= 1);
                if (e->refcnt >= 1) --e->refcnt;
                if (e->refcnt == 0) graph->release(e);
            }*/
            backprop(conf);
            return;
        }
    }

    if (conf->nsuccs == 0) {
        /* for (Edge *e: sucs) {
             assert(e->refcnt <= 1);
             if (e->refcnt >= 1) --e->refcnt;
             if (e->refcnt == 0) graph->release(e);
         }*/
        assign_value(conf, CZERO);
        backprop(conf);
        return;
    }

    conf->sucs = SuccessorQueue{sucs.data(), (uint32_t) sucs.size()};
    _dstack.push(conf);
    conf->instack = true;
}

void Algorithm::CZCycleDetectFPA::backprop(DependencyGraph::Configuration *c) {
    assert(c->isDone());
    std::unordered_set<Configuration *> W;
    W.insert(c);
    while (!W.empty()) {
        auto vit = W.begin();
        auto v = *vit;
        assert(v == root || dependent_search(v, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
        assert(v->isDone() || v->instack); // bad assert
        auto pit = v->dependency_set.before_begin();
        auto it = v->dependency_set.begin();
        while (it != v->dependency_set.end()) {
            auto &e = *it;
            if (!e->source->isDone()) {
                ((e->is_negated) ? _processedNegationEdges : _processedEdges) += 1;
                auto [_, a] = eval_edge(e);
                if (a == ONE || a == CZERO) {
                    // backpropagated assignment; keep going!
                    W.insert(e->source);
                    //v->dependency_set.erase_after(pit);
                    it = pit;
                } else {
                    assert(e->source == root || e->source->isDone() || dependent_search(e->source, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
                    e->source->resucs.push_back(e);
                    if (!e->source->instack) {
                        e->source->recheck = true;
                    }
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

std::pair<Configuration *, Assignment> Algorithm::CZCycleDetectFPA::eval_edge(DependencyGraph::Edge *e) {
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
                assign_value(e->source, CZERO);
                a = CZERO;
            }
        } else if (allOne) {
            assign_value(e->source, ONE);
            a = ONE;
        }
    } else { // is_negated
        if (hasCZero) {
            assign_value(e->source, ONE);
            a = ONE;
        } else if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            if (e->source->nsuccs == 0) {
                assign_value(e->source, CZERO);
                a = CZERO;
            }
        } else if (e->processed && retval->assignment == ZERO) {
            assign_value(e->source, ONE);
        }
    }
    return std::make_pair(retval, a);
}

void Algorithm::CZCycleDetectFPA::assign_value(DependencyGraph::Configuration *c, DependencyGraph::Assignment a) {
    assert(!c->isDone());
    assert(a == ONE || a == CZERO);
    c->assignment = a;
    c->nsuccs = 0;
    //backprop(c);
}

template <typename GoodPred, typename BadPred>
bool Algorithm::CZCycleDetectFPA::dependent_search(const Configuration* c, GoodPred&& pred, BadPred&& bad) const
{
    light_deque<const Configuration *> W;
    std::unordered_set<const Configuration *> passed;
    W.push_back(c);
    bool good = false;
    while (!W.empty()) {
        auto v = W.back(); W.pop_back();
        if (passed.find(v) != std::end(passed))
            continue;
        for (const auto dep : v->dependency_set) {
            auto d = dep->source;
            if (pred(d)) {
                return true;
            }
            else if (!bad(d)) {
                W.push_back(d);
            }
            passed.insert(v);
        }
    }

    return good;
}

