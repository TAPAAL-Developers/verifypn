
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/SearchStrategy/SearchStrategy.h"

#include <iostream>
#include <cassert>

namespace CTL::SearchStrategy {

    bool SearchStrategy::empty() const
    {
        return waiting_size() == 0 && _negation.empty() && _dependencies.empty();
    }

    void SearchStrategy::push_negation(DependencyGraph::Edge* edge)
    {
        edge->_status = 3;
        ++edge->_refcnt;
        bool allOne = true;
        bool hasCZero = false;

        for (DependencyGraph::Configuration *c : edge->_targets) {
            if (c->_assignment == DependencyGraph::Assignment::CZERO) {
                hasCZero = true;
                break;
            }
            if (c->_assignment != DependencyGraph::Assignment::ONE) {
                allOne = false;
            }
        }

        if(allOne || hasCZero)
        {
            _dependencies.push_back(edge);
        }
        else
        {
            _negation.push_back(edge);
        }
    }

    void SearchStrategy::push_edge(DependencyGraph::Edge *edge)
    {
        if(edge->_status > 0 || edge->_source->is_done()) return;
        if(edge->_processed && edge->_is_negated)
        {
            push_negation(edge);
            return;
        }
        edge->_status = 1;
        ++edge->_refcnt;
        push_to_waiting(edge);
    }

    void SearchStrategy::push_dependency(DependencyGraph::Edge* edge)
    {
        if(edge->_source->is_done()) return;
        edge->_status = 2;
        ++edge->_refcnt;
        _dependencies.push_back(edge);
    }

    DependencyGraph::Edge* SearchStrategy::pop_edge(bool saturate)
    {
        if(saturate && _dependencies.empty()) return nullptr;

        if (waiting_size() == 0 && _dependencies.empty()) {
            return nullptr;
        }

        auto edge = _dependencies.empty() ? pop_from_waiting() : _dependencies.back();

        if(!_dependencies.empty())
            _dependencies.pop_back();

        assert(edge->_refcnt >= 0);
        --edge->_refcnt;
        edge->_status = 0;
        return edge;
    }

    uint32_t SearchStrategy::max_distance() const
    {
        uint32_t m = 0;
        for(DependencyGraph::Edge* e : _negation)
        {
            if(!e->_source->is_done())
                m = std::max(m, e->_source->get_distance());
        }
        return m;
    }

    void SearchStrategy::release_negation_edges(uint32_t dist)
    {
        for(auto it = _negation.begin(); it != _negation.end(); ++it)
        {
            assert(*it);
            if((*it)->_source->get_distance() >= dist || (*it)->_source->is_done())
            {
                push_to_waiting(*it);
                it = _negation.erase(it);
                if(_negation.empty() || it == _negation.end()) break;
            }
        }
    }

    bool SearchStrategy::trivial_negation()
    {
        for(auto it = _negation.begin(); it != _negation.end(); ++it)
        {
            bool allOne = true;
            bool hasCZero = false;
            auto e = *it;
            for (DependencyGraph::Configuration *c : e->_targets) {
                if (c->_assignment == DependencyGraph::Assignment::CZERO) {
                    hasCZero = true;
                    break;
                }
                if (c->_assignment != DependencyGraph::Assignment::ONE) {
                    allOne = false;
                }
            }

            if(allOne || hasCZero)
            {
                _dependencies.push_back(*it);
                it = _negation.erase(it);
                if(_negation.empty() || it == _negation.end()) break;
            }
        }
        return !_dependencies.empty();
    }
}