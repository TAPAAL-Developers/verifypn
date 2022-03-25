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
        auto& c = _dstack.back();
        if (c._config->isDone()) { // TODO fix
            c._config->instack = false;
            _dstack.pop_back();
            continue;
        }
        else if(c._edges.empty())
        {
            c._config->instack = false;
            _dstack.pop_back();
            continue;
        }
#ifndef NDEBUG
        assert(c._config == root || dependent_search(c._config, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
        assert(c._config == root || dependent_search(c._config, [&](auto c) { return c == root; }, [] (Configuration* c) { return c->isDone(); }));
#endif

        auto* e = c.pop_edge(); // do the pop when
        if (e == nullptr) continue;
        if (e->handled)
            continue;
        assert(e->source == c._config);

        auto [next, _] = eval_edge(e);
        ((e->is_negated) ? _processedNegationEdges : _processedEdges) += 1;
        //assert(next != nullptr || c->isDone());

        if (c._config->isDone()) {
            backprop(c._config);
        } else if (next != nullptr && !next->isDone()) {
            next->addDependency(e);
            if (next->assignment == UNKNOWN || next->recheck) {
                assert(next->rank == std::numeric_limits<uint64_t>::max() || next->recheck);
                push_edge(next);
                next->recheck = false;
                next->rank = c->rank + 1;
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
            backprop(conf);
            return;
        }
    }

    if (conf->nsuccs == 0) {
        assign_value(conf, CZERO);
        backprop(conf);
        return;
    }

    _dstack.emplace_back({conf, std::move(sucs)});
    conf->instack = true;
}

void Algorithm::CZCycleDetectFPA::backprop(DependencyGraph::Configuration *c) {
    assert(c->isDone());
    std::vector<DependencyGraph::Configuration*> waiting{c};
    while (!waiting.empty()) {
        auto* v = waiting.back();
        waiting.pop_back();
        assert(v == root || dependent_search(v, [&](auto c) { return c->instack; }, [] (Configuration* c) { return c->isDone(); }));
        if(!v->isDone()) continue;
        for(auto* e : v->depedency_set)
        {
            if (!e->source->isDone()) {
                ((e->is_negated) ? _processedNegationEdges : _processedEdges) += 1;
                auto [_, a] = eval_edge(e);
                if (a == ONE || a == CZERO) {
                    waiting.insert(e->source);
                } else {
                    assert(e->source == root || e->source->isDone() ||
                    dependent_search(e->source,
                        [&](auto c) { return c->instack; },
                            [] (Configuration* c) { return c->isDone(); }));
                }
            }
        }
        v->dependency_set.clear();
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

