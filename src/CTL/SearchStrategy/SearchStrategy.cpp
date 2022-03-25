
#include "CTL/DependencyGraph/Configuration.h"
#include "CTL/SearchStrategy/SearchStrategy.h"

#include <iostream>
#include <cassert>

namespace SearchStrategy{

    bool SearchStrategy::empty() const
    {
        return Wsize() == 0 && N.empty() && D.empty();
    }

    void SearchStrategy::pushNegation(DependencyGraph::Edge* edge)
    {
        edge->status = DependencyGraph::EdgeStatus::Negation;
        ++edge->refcnt;
        bool allOne = true;
        bool hasCZero = false;

        for (DependencyGraph::Configuration *c : edge->targets) {
            if (c->assignment == DependencyGraph::Assignment::CZERO) {
                hasCZero = true;
                break;
            }
            if (c->assignment != DependencyGraph::Assignment::ONE) {
                allOne = false;
            }
        }

        if(allOne || hasCZero)
        {
            D.push_back(edge);
        }
        else
        {
            if (isdfs_) {
                pushToW(edge);
            }
            else {
                N.push_back(edge);
            }
        }
    }

    void SearchStrategy::pushEdge(DependencyGraph::Edge *edge)
    {
        if(edge->status != DependencyGraph::EdgeStatus::NotWaiting || edge->source->isDone()) return;
        if(edge->processed && edge->is_negated)
        {
            pushNegation(edge);
            return;
        }
        edge->status = DependencyGraph::EdgeStatus::InWaiting;
        ++edge->refcnt;
        pushToW(edge);
    }

    void SearchStrategy::pushDependency(DependencyGraph::Edge* edge)
    {
        if(edge->source->isDone()) return;
        edge->status = DependencyGraph::EdgeStatus::Dependency;
        ++edge->refcnt;
        D.push_back(edge);
    }

    std::pair<DependencyGraph::Edge*, bool> SearchStrategy::popEdge(bool saturate)
    {
        if(saturate && D.empty()) return std::make_pair(nullptr, false);

        if (Wsize() == 0 && D.empty()) {
            return std::make_pair(nullptr, false);
        }

        auto edge = D.empty() ? popFromW() : D.back();
        bool was_dep = !D.empty();
        if(!D.empty())
            D.pop_back();

        assert(edge->refcnt >= 0);
        --edge->refcnt;
        edge->status = DependencyGraph::EdgeStatus::NotWaiting;
        return std::make_pair(edge, was_dep);
    }

    uint32_t SearchStrategy::maxDistance() const
    {
        uint32_t m = 0;
        for(DependencyGraph::Edge* e : N)
        {
            if(!e->source->isDone())
                m = std::max(m, e->source->getDistance());
        }
        return m;
    }

    bool SearchStrategy::available() const
    {
        return Wsize() > 0 || !D.empty();
    }

    void SearchStrategy::releaseNegationEdges(uint32_t dist)
    {
        for(auto it = N.begin(); it != N.end(); ++it)
        {
            assert(*it);
            if((*it)->source->getDistance() >= dist || (*it)->source->isDone())
            {
                pushToW(*it);
                it = N.erase(it);
                if(N.empty() || it == N.end()) break;
            }
        }
    }

    bool SearchStrategy::trivialNegation()
    {
        for(auto it = N.begin(); it != N.end(); ++it)
        {
            bool allOne = true;
            bool hasCZero = false;
            auto e = *it;
            for (DependencyGraph::Configuration *c : e->targets) {
                if (c->assignment == DependencyGraph::Assignment::CZERO) {
                    hasCZero = true;
                    break;
                }
                if (c->assignment != DependencyGraph::Assignment::ONE) {
                    allOne = false;
                }
            }

            if(allOne || hasCZero)
            {
                D.push_back(*it);
                it = N.erase(it);
                if(N.empty() || it == N.end()) break;
            }
        }
        return !D.empty();
    }
}
