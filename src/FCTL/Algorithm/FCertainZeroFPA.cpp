#include "FCTL/Algorithm/FCertainZeroFPA.h"

#include <cassert>
#include <iostream>


namespace Featured {
    using namespace DependencyGraph;

    bool Algorithm::FCertainZeroFPA::search(DependencyGraph::BasicDependencyGraph& t_graph) {
        graph = &t_graph;


        root = graph->initialConfiguration();
        {
            explore(root);
        }

        size_t cnt = 0;
        while (!strategy->empty()) {
            while (auto e = strategy->popEdge(false)) {
                ++e->refcnt;
                assert(e->refcnt >= 1);
                checkEdge(e);
                assert(e->refcnt >= -1);
                if (e->refcnt > 0) --e->refcnt;
                if (e->refcnt == 0) graph->release(e);
                ++cnt;
                //if ((cnt % 1000) == 0) strategy->trivialNegation();
                if (root->isDone() || root->bad != bddfalse) return root->good == bddtrue;
            }

            if (root->isDone() || root->bad != bddfalse) return root->good == bddtrue;

            // TODO check if this function does anything unsound
            if (!strategy->trivialNegation()) {
                cnt = 0;
                strategy->releaseNegationEdges(strategy->maxDistance());
                continue;
            }
        }

        return root->assignment == ONE;
    }

    void Algorithm::FCertainZeroFPA::push_dependencies(const Configuration* c) {
        for (auto* e: c->dependency_set) {
            if (e->is_negated) {
                strategy->pushNegation(e);
            }
            else {
                strategy->pushDependency(e);
            }
        }
    }

    std::pair<bool, Configuration*> Algorithm::FCertainZeroFPA::evaluate_assignment(Edge* e) {
        auto* src = e->source;

        if (e->is_negated) {
            auto* target = e->targets.begin()->conf;
            if (!e->processed) {
                return std::make_pair(false, target);
            }
            else {
                src->good = target->bad;
                src->bad = target->good;
                return std::make_pair(true, nullptr);
            }
            assert(false);
        }
        bdd good = bddtrue;
        bdd bad = bddfalse;
        Configuration* ret = nullptr;
        // AND over all targets of edge
        for (auto &[suc, feat] : e->targets) {
            good &= feat & suc->good;
            // OR over all targets of this edge
            e->bad_iter |= bdd_imp(feat, suc->bad);
            if (!suc->checked && ret == nullptr) {
                ret = suc;
            }
        }
        // partial OR of all edges from good (just add info from this edge)
        good = good | src->good;

        // AND over bad from all successor edges
        for (auto suc: e->source->successors) {
            bad |= suc->bad_iter;
        }
        bool updated = false;
        if (bdd_imp(src->good, good) == bddtrue) {
            src->good = good;
            updated = true;
        }
        if (bdd_imp(src->bad, bad) == bddtrue) {
            src->bad = bad;
            updated = true;
        }
        return std::make_pair(updated, ret);
    }

    void Algorithm::FCertainZeroFPA::checkEdge(Edge* e, bool only_assign) {
        if (e->handled) return;
        if (e->source->isDone()) {
            if (e->refcnt == 0) graph->release(e);
            return;
        }
        e->processed = true;

        auto [updated, undecided] = evaluate_assignment(e);

        if (updated) {
            push_dependencies(e->source);
        }
        if (undecided != nullptr) {
            explore(undecided);
        }
        if (e->refcnt == 0) graph->release(e);
    }

    void Algorithm::FCertainZeroFPA::explore(Configuration* c) {
        c->assignment = ZERO;

        {
            auto succs = graph->successors(c);
            /*
             * Initialising A+ and A-
             */
            if (succs.empty()) {
                assert(c->good == bddfalse);
                c->bad = bddtrue;
                push_dependencies(c);
                return;
            }
            // either all negated or none negated
            assert(std::min(std::count_if(std::begin(succs), std::end(succs),
                                     [](const auto& e){ return e->is_negated; }),
                            std::count_if(std::begin(succs), std::end(succs),
                                     [](const auto& e){ return !e->is_negated; })) == 0);

            for (int32_t i = c->nsuccs - 1; i >= 0; --i) {
                auto [updated, _] = evaluate_assignment(succs[i]);
                // TODO check if edge still valid, if not then skip it
                if (updated) {
                    push_dependencies(c);
                }
                if (c->isDone()) {
                    return;
                }
            }
            for (auto* e: succs) {
                strategy->pushEdge(e);
            }
            _exploredConfigurations += 1;
            _numberOfEdges += c->nsuccs;
            // before we start exploring, lets check if any of them determine
            // the outcome already!

            // for (int32_t i = c->nsuccs - 1; i >= 0; --i) {
            //     checkEdge(succs[i], true);
            //     if (c->isDone()) {
            //         for (Edge* e: succs) {
            //             assert(e->refcnt <= 1);
            //             if (e->refcnt >= 1) --e->refcnt;
            //             if (e->refcnt == 0) graph->release(e);
            //         }
            //         return;
            //     }
            // }

            // if (c->nsuccs == 0) {
            //     for (Edge* e: succs) {
            //         assert(e->refcnt <= 1);
            //         if (e->refcnt >= 1) --e->refcnt;
            //         if (e->refcnt == 0) graph->release(e);
            //     }
            //     finalAssign(c, CZERO);
            //     return;
            // }

            // for (Edge* succ: succs) {
            //     assert(succ->refcnt <= 1);
            //     if (succ->refcnt > 0) {
            //         strategy->pushEdge(succ);
            //         --succ->refcnt;
            //         if (succ->refcnt == 0) graph->release(succ);
            //     } else if (succ->refcnt == 0) {
            //         graph->release(succ);
            //     }
            // }
        }
        strategy->flush();
    }
}
