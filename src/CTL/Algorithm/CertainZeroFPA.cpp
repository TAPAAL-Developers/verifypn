#include "CTL/Algorithm/CertainZeroFPA.h"

#include <cassert>
#include <iostream>

namespace CTL::Algorithm {

using namespace DependencyGraph;
using namespace SearchStrategy;

auto CertainZeroFPA::search(DependencyGraph::BasicDependencyGraph &t_graph) -> bool {
    _graph = &t_graph;

    _vertex = _graph->initial_configuration();
    { explore(_vertex); }

    size_t cnt = 0;
    while (!_strategy->empty()) {
        while (auto e = _strategy->pop_edge(false)) {
            ++e->_refcnt;
            assert(e->_refcnt >= 1);
            check_edge(e);
            assert(e->_refcnt >= -1);
            if (e->_refcnt > 0)
                --e->_refcnt;
            if (e->_refcnt == 0)
                _graph->release(e);
            ++cnt;
            if ((cnt % 1000) == 0)
                _strategy->trivial_negation();
            if (_vertex->is_done())
                return _vertex->_assignment == ONE;
        }

        if (_vertex->is_done())
            return _vertex->_assignment == ONE;

        if (!_strategy->trivial_negation()) {
            cnt = 0;
            _strategy->release_negation_edges(_strategy->max_distance());
            continue;
        }
    }

    return _vertex->_assignment == ONE;
}

void CertainZeroFPA::check_edge(Edge *e, bool only_assign) {
    if (e->_handled)
        return;
    if (e->_source->is_done()) {
        if (e->_refcnt == 0)
            _graph->release(e);
        return;
    }

    /*{
        bool any = false;
        size_t n = 0;
        size_t k = 0;
        for(Edge* deps : e->source->dependency_set)
        {
            ++k;
            if(deps->source->isDone()) continue;
            any = true;
            ++n;
        }
        if(!any && e->source != vertex) return;
    }*/

    bool allOne = true;
    bool hasCZero = false;
    // auto pre_empty = e->targets.empty();
    Configuration *lastUndecided = nullptr;
    {
        auto it = e->_targets.begin();
        auto pit = e->_targets.before_begin();
        while (it != e->_targets.end()) {
            if ((*it)->_assignment == ONE) {
                e->_targets.erase_after(pit);
                it = pit;
            } else {
                allOne = false;
                if ((*it)->_assignment == CZERO) {
                    hasCZero = true;
                    // assert(e->assignment == CZERO || only_assign);
                    break;
                } else if (lastUndecided == nullptr) {
                    lastUndecided = *it;
                }
            }
            pit = it;
            ++it;
        }
    }
    /*if(e->targets.empty())
    {
        assert(e->assignment == ONE || e->children == 0);
    }*/

    if (e->_is_negated) {
        _processedNegationEdges += 1;
        // Process negation edge
        if (allOne) {
            --e->_source->_nsuccs;
            e->_handled = true;
            assert(e->_refcnt > 0);
            if (only_assign)
                --e->_refcnt;
            if (e->_source->_nsuccs == 0) {
                final_assign(e, CZERO);
            }
            if (e->_refcnt == 0) {
                _graph->release(e);
            }
        } else if (hasCZero) {
            final_assign(e, ONE);
        } else {
            assert(lastUndecided != nullptr);
            if (only_assign)
                return;
            if (lastUndecided->_assignment == ZERO && e->_processed) {
                final_assign(e, ONE);
            } else {
                if (!e->_processed) {
                    _strategy->push_negation(e);
                }
                lastUndecided->add_dependency(e);
                if (lastUndecided->_assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
    } else {
        _processedEdges += 1;
        // Process hyper edge
        if (allOne) {
            final_assign(e, ONE);
        } else if (hasCZero) {
            --e->_source->_nsuccs;
            e->_handled = true;
            assert(e->_refcnt > 0);
            if (only_assign)
                --e->_refcnt;
            if (e->_source->_nsuccs == 0) {
                final_assign(e, CZERO);
            }
            if (e->_refcnt == 0) {
                _graph->release(e);
            }

        } else if (lastUndecided != nullptr) {
            if (only_assign)
                return;
            if (!e->_processed) {
                if (!lastUndecided->is_done()) {
                    for (auto t : e->_targets)
                        t->add_dependency(e);
                }
            }
            if (lastUndecided->_assignment == UNKNOWN) {
                explore(lastUndecided);
            }
        }
    }
    if (e->_refcnt > 0 && !only_assign)
        e->_processed = true;
    if (e->_refcnt == 0)
        _graph->release(e);
}

void CertainZeroFPA::final_assign(DependencyGraph::Edge *e, DependencyGraph::assignment_e a) {
    final_assign(e->_source, a);
}

void CertainZeroFPA::final_assign(DependencyGraph::Configuration *c,
                                  DependencyGraph::assignment_e a) {
    assert(a == ONE || a == CZERO);

    c->_assignment = a;
    c->_nsuccs = 0;
    for (DependencyGraph::Edge *e : c->_dependency_set) {
        if (!e->_source->is_done()) {
            /*if (a == CZERO) {
                e->assignment = CZERO;
            } else if (a == ONE) {
                assert(e->children >= 1);
                --e->children;
                if(e->children == 0)
                    e->assignment = ONE;
            }*/
            if (!e->_is_negated || a == CZERO) {
                _strategy->push_dependency(e);
            } else {
                _strategy->push_negation(e);
            }
        }
        assert(e->_refcnt > 0);
        --e->_refcnt;
        if (e->_refcnt == 0)
            _graph->release(e);
    }

    c->_dependency_set.clear();
}

void CertainZeroFPA::explore(Configuration *c) {
    c->_assignment = ZERO;

    {
        auto succs = _graph->successors(c);
        c->_nsuccs = succs.size();

        _exploredConfigurations += 1;
        _numberOfEdges += c->_nsuccs;
        // before we start exploring, lets check if any of them determine
        // the outcome already!

        for (int64_t i = c->_nsuccs - 1; i >= 0; --i) {
            check_edge(succs[i], true);
            if (c->is_done()) {
                for (Edge *e : succs) {
                    assert(e->_refcnt <= 1);
                    if (e->_refcnt >= 1)
                        --e->_refcnt;
                    if (e->_refcnt == 0)
                        _graph->release(e);
                }
                return;
            }
        }

        if (c->_nsuccs == 0) {
            for (Edge *e : succs) {
                assert(e->_refcnt <= 1);
                if (e->_refcnt >= 1)
                    --e->_refcnt;
                if (e->_refcnt == 0)
                    _graph->release(e);
            }
            final_assign(c, CZERO);
            return;
        }

        for (Edge *succ : succs) {
            assert(succ->_refcnt <= 1);
            if (succ->_refcnt > 0) {
                _strategy->push_edge(succ);
                --succ->_refcnt;
                if (succ->_refcnt == 0)
                    _graph->release(succ);
            } else if (succ->_refcnt == 0) {
                _graph->release(succ);
            }
        }
    }
    _strategy->flush();
}
} // namespace CTL::Algorithm