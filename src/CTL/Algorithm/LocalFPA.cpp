#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/DependencyGraph/Edge.h"

#include <cassert>
#include <iostream>

namespace CTL::Algorithm {

bool LocalFPA::search(DependencyGraph::BasicDependencyGraph &t_graph) {
    using namespace DependencyGraph;
    _graph = &t_graph;

    Configuration *v = _graph->initial_configuration();
    explore(v);

    while (!_strategy->empty()) {
        while (auto e = _strategy->pop_edge()) {

            if (v->_assignment == DependencyGraph::ONE) {
                break;
            }

            bool allOne = true;
            Configuration *lastUndecided = nullptr;

            for (DependencyGraph::Configuration *c : e->_targets) {
                if (c->_assignment != DependencyGraph::ONE) {
                    allOne = false;
                    lastUndecided = c;
                }
            }

            if (e->_is_negated) {
                _processedNegationEdges += 1;
                // Process negation edge
                if (allOne) {
                } else if (!e->_processed) {
                    add_dependency(e, lastUndecided);
                    if (lastUndecided->_assignment == UNKNOWN) {
                        explore(lastUndecided);
                    }
                    _strategy->push_negation(e);
                } else {
                    final_assign(e->_source, ONE);
                }

            } else {
                _processedEdges += 1;
                // Process hyper edge
                if (allOne) {
                    final_assign(e->_source, ONE);
                } else {
                    add_dependency(e, lastUndecided);
                    if (lastUndecided->_assignment == UNKNOWN) {
                        explore(lastUndecided);
                    }
                }
            }
            e->_processed = true;
            if (e->_refcnt == 0)
                _graph->release(e);
        }
        if (!_strategy->trivial_negation()) {
            _strategy->release_negation_edges(_strategy->max_distance());
        }
    }

    return v->_assignment == ONE;
}

void LocalFPA::final_assign(DependencyGraph::Configuration *c, DependencyGraph::Assignment a) {
    assert(a == DependencyGraph::ONE);
    c->_assignment = a;

    for (DependencyGraph::Edge *e : c->_dependency_set) {
        if (e->_is_negated) {
            _strategy->push_negation(e);
        } else {
            _strategy->push_dependency(e);
        }
        --e->_refcnt;
        if (e->_refcnt == 0)
            _graph->release(e);
    }

    c->_dependency_set.clear();
}

void LocalFPA::explore(DependencyGraph::Configuration *c) {
    assert(c->_assignment == DependencyGraph::UNKNOWN);
    c->_assignment = DependencyGraph::ZERO;
    auto succs = _graph->successors(c);

    for (DependencyGraph::Edge *succ : succs) {
        _strategy->push_edge(succ);
        --succ->_refcnt;
        if (succ->_refcnt == 0)
            _graph->release(succ);
    }

    _exploredConfigurations += 1;
    _numberOfEdges += succs.size();
}

void LocalFPA::add_dependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target) {
    target->add_dependency(e);
}

} // namespace CTL::Algorithm
