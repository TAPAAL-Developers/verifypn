#include "FCTL/Algorithm/FCertainZeroFPA.h"

#include <cassert>
#include <iostream>


#include "logging.h"


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
        if (!e->is_negated) {
            os << ", {";
            print_edge_targets(e, os) << "})";
        }
        else {
            os << " --> " << (*e->targets.begin()).conf->id << ")";
        }
print_edge_targets(e, os) << "})";
        return os;
    }

    std::map<std::string,std::string> correct;

    std::pair<std::string,std::string> split(const std::string& line) {
        for(int64_t i = line.size() - 1; i > 0; --i)
        {
            if(line[i] == ',')
                return std::make_pair(std::string(line.data(), i), std::string(line.data() + i, (line.size() - i)));
        }
        return {"",""};
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
                if (root->done() || root->bad != bddfalse) return root->good.id() == bddtrue.id();
            }

            if (root->done() || root->bad != bddfalse) return root->good.id() == bddtrue.id();

            // TODO check if this function does anything unsound
            if (!strategy->trivialNegation()) {
                cnt = 0;
                strategy->releaseNegationEdges(strategy->maxDistance());
                continue;
            }
        }

        return root->good == bddtrue;
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
            ++_processedNegationEdges;
            auto* target = e->targets.begin()->conf;
            /*if (target->unimproved()) {
                if (!target->is_seen()) {
                    explore(target);
                }
                if (!e->processed) {
                    strategy->pushNegation(e);
                }
                target->addDependency(e);
            }*/

            if (target->assignment != UNKNOWN && e->processed) {
                // all good ones are those that are not good below
                // all bad ones are those that were not proven good.
                auto good = !target->good;
                auto bad = !good;
                return {nullptr, good, bad};

            } else {
                //if (!e->processed) {
                strategy->pushNegation(e);
                //}
                assert(target->good.id() == bddfalse.id() && target->bad.id() == bddfalse.id());
                return {target, bddfalse, bddfalse};
                if ((src->good < target->bad) == bddtrue || (src->bad < target->good) == bddtrue) {
                    src->good = target->bad;
                    src->bad = target->good;
                }
            }
            assert(false);
        }
        ++_processedEdges;

        /**
         * Default values here may look strange; shouldn't they be different?
         * Answer is no: in fact we compute both as conjunctions.
         *   Good is a DNF over hyperedges and their targets,
         *     so we compute just the conj over current edge targets.
         *   Bad is a CNF over hyperedges and their targets,
         *     so we cache the value of the disjunction over targets for each edge,
         *     then compute the full conjunction here.
         */
        bdd good = bddtrue;
        bdd bad = bddtrue;
        Configuration* ret = nullptr;
        // AND over all targets of edge
        for (auto& [suc, feat]: e->targets) {
            good &= feat & suc->good;
            // OR over all targets of this edge
            e->bad_iter |= bdd_imp(feat, suc->bad);
            if (ret == nullptr || *suc < *ret) {
                ret = suc;
            }
            // if ((!suc->done() && ret == nullptr) ||
            //     (!suc->is_seen() && ret != nullptr && ret->is_seen())) {
            //     ret = suc;
            // }
        }
        // partial OR of all edges from good (just add info from this edge)
        good = good | src->good;

        // AND over bad from all successor edges
        for (auto suc: e->source->successors) {
            bad &= suc->bad_iter;
        }
        return {ret, good, bad};
    }

    bool Algorithm::FCertainZeroFPA::try_update(DependencyGraph::Configuration* c, bdd good, bdd bad) {
        assert((c->good >> good).id() == bddtrue.id() && (c->bad >> bad).id() == bddtrue.id());

        if (c->good.id() != good.id()) {
            assert(c->good < good == bddtrue);
        }

        if ((c->good < good).id() == bddtrue.id() || (c->bad < bad).id() == bddtrue.id()) {
#if DEBUG_DETAILED
            std::cout << "Assign: " << c->id
                      << ", good: " << c->good.id() << " => " << good.id()
                      << "; bad: " << c->bad.id() << " => " << bad.id() << std::endl;
#endif
            c->good = good;
            c->bad = bad;
            if (c->good.id() == bddtrue.id()) {
                c->assignment = ONE;
            }
            if (c->bad.id() == bddtrue.id()) {
                c->assignment = CZERO;
            }
            //push_dependencies(c);
#if DEBUG_DETAILED
            std::cerr << "ASSIGN: [" << c->id << "]: " << to_string(static_cast<Assignment>(c->assignment)) << "\n";

            if (correct.empty()) {
                std::fstream in;
                in.open("/tmp/dump", std::ios::in);
                if (in) {
                    std::string line;
                    while (getline(in, line)) {
                        auto [a, b] = split(line);
                        correct[a] = b;
                    }
                }
            }

            if (c->done()) {
                std::stringstream ss;
                graph->print(c, ss);
                auto [a, b] = split(ss.str());
                if (correct.count(a)) {
                    if (correct[a] != b) {
                        std::cerr << "Error on :" << a << "\n\tGOT" << b << " expected " << correct[a] << std::endl;
                        assert(false);
                    } else
                        std::cerr << "¤ OK (" << c->id << ") = " << DependencyGraph::to_string((Assignment) c->assignment)
                                  << std::endl;
                } else
                    std::cerr << "¤ NO MATCH (" << c->id << ") " << DependencyGraph::to_string((Assignment) c->assignment)
                              << std::endl;
            }
#endif
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


        auto [undecided, good, bad] = evaluate_assignment(e);

        if (try_update(src, good, bad)) {
            push_dependencies(src);
        }
        if (undecided != nullptr) {
            undecided->addDependency(e);
            if (!undecided->is_seen()) {
                explore(undecided);
            }
        }
        if (e->refcnt > 0)  e->processed = true;
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
                auto ret = try_update(c, bddfalse, bddtrue);
                assert(ret);
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
                for (auto suc: succs) {
                    bad &= suc->bad_iter; // ad hoc update copied from evaluate_assignment;
                                          // bad is vacuously true as successors unassigned at this point.
                }
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
                c->successors.push_front(e);
            }

        }
        strategy->flush();
    }
}