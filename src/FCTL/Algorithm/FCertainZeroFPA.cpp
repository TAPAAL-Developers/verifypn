#include "FCTL/Algorithm/FCertainZeroFPA.h"

#include <cassert>
#include <iostream>

#if 1// || !defined(NDEBUG)
# define DEBUG_DETAILED 1
#else
# define DEBUG_DETAILED 0
#endif


namespace Featured {
    using namespace DependencyGraph;

#if DEBUG_DETAILED
    std::ostream& print_suc_pair(const Edge::Successor& suc, std::ostream& os = std::cout) {
        return os << "(" << suc.conf->id << "," << suc.feat.id() << ") ";
    }

    std::ostream& print_edge_targets(const Edge* e, std::ostream& os = std::cout) {
        os << "  ";
        for (auto& sucinfo: e->targets) {
            print_suc_pair(sucinfo, os);
        }
        return os;
    }

    std::ostream& print_edge(const Edge* e, std::ostream& os = std::cout) {
        os << "(" << e->source->id << ", {";
        print_edge_targets(e, os) << "})";
        return os;
    }

#endif

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
                if (root->done() || root->bad != bddfalse) return root->good == bddtrue;
            }

            if (root->done() || root->bad != bddfalse) return root->good == bddtrue;

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
            } else {
                strategy->pushDependency(e);
            }
        }
    }

    Algorithm::FCertainZeroFPA::Evaluation Algorithm::FCertainZeroFPA::evaluate_assignment(Edge* e) {
        auto* src = e->source;

        if (e->is_negated) {
            auto* target = e->targets.begin()->conf;
            auto good = target->bad;
            auto bad = target->good;
            if (!e->targets.begin()->conf->is_seen()) {
                return {target, good, bad};
            } else {
                return {nullptr, good, bad};
                if ((src->good < target->bad) == bddtrue || (src->bad < target->good) == bddtrue) {
                    src->good = target->bad;
                    src->bad = target->good;
                }
            }
            assert(false);
        }
        bdd good = bddtrue;
        bdd bad = bddfalse;
        Configuration* ret = nullptr;
        // AND over all targets of edge
        for (auto& [suc, feat]: e->targets) {
            good &= feat & suc->good;
            // OR over all targets of this edge
            e->bad_iter |= bdd_imp(feat, suc->bad);
            if (!suc->is_seen() && ret == nullptr) {
                ret = suc;
            }
        }
        // partial OR of all edges from good (just add info from this edge)
        good = good | src->good;

        // AND over bad from all successor edges
        for (auto suc: e->source->successors) {
            bad |= suc->bad_iter;
        }
        return {ret, good, bad};
    }

    bool Algorithm::FCertainZeroFPA::try_update(DependencyGraph::Configuration* c, bdd good, bdd bad) {
        if ((c->good < good) == bddtrue || (c->bad < bad) == bddtrue) {
#if DEBUG_DETAILED
            std::cout << "Assign: " << c->id
                      << ", good: " << c->good.id() << " => " << good.id()
                      << "; bad: " << c->bad.id() << " => " << bad.id() << std::endl;
#endif
            c->good = good;
            c->bad = bad;
            //push_dependencies(c);
            return true;
        }
        else return false;
    }

    void Algorithm::FCertainZeroFPA::checkEdge(Edge* e, bool only_assign) {
        if (e->handled) return;
        if (e->source->done()) {
            if (e->refcnt == 0) graph->release(e);
            return;
        }
        auto* src = e->source;
#if DEBUG_DETAILED
        std::cout << "Checking edge: ";
        print_edge(e) << std::endl;
#endif

        e->processed = true;

        auto [undecided, good, bad] = evaluate_assignment(e);

        if (try_update(src, good, bad)) {
            push_dependencies(src);
        }
        if (undecided != nullptr && !src->done()) {
            explore(undecided);
        }
        if (e->refcnt == 0) graph->release(e);
    }


    void Algorithm::FCertainZeroFPA::explore(Configuration* c) {
        c->seen_ = true;

        {
            auto succs = graph->successors(c);
            c->nsuccs = succs.size();

#if DEBUG_DETAILED
            std::cout << "Succs of " << c->id << ": \n";
            for (auto suc: succs) {
                std::cout << "  "; print_edge(suc, std::cout) << "\n";
            }
#endif
            _exploredConfigurations += 1;
            _numberOfEdges += c->nsuccs;
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
                                          [](const auto& e) { return e->is_negated; }),
                            std::count_if(std::begin(succs), std::end(succs),
                                          [](const auto& e) { return !e->is_negated; })) == 0);

            for (int32_t i = c->nsuccs - 1; i >= 0; --i) {
                auto [_, good, bad] = evaluate_assignment(succs[i]);
                // TODO check if edge still valid, if not then skip it
                if (try_update(c, good, bad)) {
                    push_dependencies(c);
                }
                if (c->done()) {
                    return;
                }
            }
            for (auto* e: succs) {
                strategy->pushEdge(e);
            }
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
