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

using namespace DependencyGraph;

bool Algorithm::CZCycleDetectFPA::search(BasicDependencyGraph &graph) {
    this->graph = &graph;
    this->root = graph.initialConfiguration();

    push_edge(root);

    while (!_dstack.empty()) {
        auto c = _dstack.top();
        if (c->isDone() || c->sucs.empty()) {
            c->sucs.clear();
            c->instack = false;
            _dstack.pop();
        }
        auto e = c->sucs.front(); c->sucs.pop();

        auto next = eval_edge(e);
        assert(next != nullptr || !c->isDone());

        if (c->isDone()) {
            backprop(c);
        }
        else {
            assert(next != nullptr && !next->isDone());
            next->addDependency(e);
            if (next->assignment == UNKNOWN) {
                assert(next->rank == std::numeric_limits<uint32_t>::max());
                push_edge(next);
                if (next->nsuccs == 1 && c->nsuccs == 1) {
                    next->rank = c->rank;
                }
                else {
                    next->rank = c->rank + 1;
                }
            }
            else {
                if (next->instack && next->rank == c->rank) {
                    assign_value(c, CZERO);
                    backprop(c);
                }
            }
        }
        if (root->isDone()) {
            return root->assignment == ONE;
        }
    }
    return root->assignment == ONE;
}

void Algorithm::CZCycleDetectFPA::push_edge(Configuration *conf) {
    assert(conf->assignment == UNKNOWN);
    assert(!conf->instack);
    auto sucs = graph->successors(conf);
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

    conf->sucs = SuccessorQueue{sucs.data(), (uint32_t)sucs.size()};
    _dstack.push(conf);
    conf->instack = true;
}

void Algorithm::CZCycleDetectFPA::backprop(DependencyGraph::Configuration *c) {
    assert(c->isDone());
    std::deque<Configuration*> W;
    W.push_back(c);
    while (!W.empty()) {
        auto v = W.front(); W.pop_front();
        for (auto e : v->dependency_set) {
            if (!e->source->isDone()) {
                eval_edge(e);
                // backpropagated assignment; keep going!
                if (e->source->isDone()) {
                    W.push_back(e->source);
                }
                else {
                    e->source->sucs.prepend(e);
                }
            }
        }
        v->dependency_set.clear();
    }
}

Configuration* Algorithm::CZCycleDetectFPA::eval_edge(DependencyGraph::Edge *e) {
    bool allOne = true, hasCZ = false;
    Configuration *retval = nullptr;
    auto it = e->targets.begin();
    auto pit = e->targets.before_begin();
    while (it != e->targets.end()) {
        if ((*it)->assignment == ONE) {
            e->targets.erase_after(pit);
            it = pit;
        } else {
            allOne = false;
            if ((*it)->assignment == CZERO) {
                hasCZ = true;
                break;
            } else if (retval == nullptr) {
                retval = *it;
            }
        }
        pit = it;
        ++it;
    }

    if (!e->is_negated) {
        if (hasCZ) {
            --e->source->nsuccs;
            e->handled = true;
            if (e->source->nsuccs == 0) {
                assign_value(e->source, CZERO);
            }
        }
        else if (allOne) {
            assign_value(e->source, ONE);
        }
    }
    else {
        if (hasCZ) {
            assign_value(e->source, ONE);
        }
        else if (allOne) {
            --e->source->nsuccs;
            e->handled = true;
            if (e->source->nsuccs == 0) {
                assign_value(e->source, CZERO);
            }
        }
    }
    return retval;
}

void Algorithm::CZCycleDetectFPA::assign_value(DependencyGraph::Configuration *c, DependencyGraph::Assignment a) {
    assert(!c->isDone());
    assert(a == ONE || a == CZERO);
    c->assignment = a;
    //backprop(c);
}



