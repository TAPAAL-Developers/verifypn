#include "FCTL/Algorithm/FCertainZeroFPA.h"

#include <cassert>
#include <iostream>

#include "logging.h"

namespace Featured {
using namespace DependencyGraph;

#if DEBUG_DETAILED
std::ostream& print_suc_pair(const Edge::Successor& suc,
                             std::ostream& os = std::cout) {
    return os << "(" << suc.conf->id << "," << suc.feat.id() << ") ";
}

std::ostream& print_edge_targets(const Edge* e, std::ostream& os = std::cout) {
    os << "  ";
    for (auto& sucinfo : e->targets) {
        print_suc_pair(sucinfo, os);
    }
    return os;
}

std::ostream& print_edge(const Edge* e, std::ostream& os = std::cout) {
    os << "(" << e->source->id << ", {";
    if (!e->is_negated) {
        os << ", {";
        print_edge_targets(e, os) << "})";
    } else {
        os << " --> " << (*e->targets.begin()).conf->id << ")";
    }
    print_edge_targets(e, os) << "})";
    return os;
}

std::map<std::string, std::string> correct;

std::pair<std::string, std::string> split(const std::string& line) {
    for (int64_t i = line.size() - 1; i > 0; --i) {
        if (line[i] == ',')
            return std::make_pair(
                std::string(line.data(), i),
                std::string(line.data() + i, (line.size() - i)));
    }
    return {"", ""};
}
#endif

bool Algorithm::FCertainZeroFPA::search(
    DependencyGraph::BasicDependencyGraph& t_graph) {
    graph = &t_graph;

    root = graph->initialConfiguration();
    { explore(root); }

    size_t cnt = 0;
    while (!strategy->empty()) {
        while (auto e = strategy->popEdge(false)) {
            ++e->refcnt;
            assert(e->refcnt >= 1);
            checkEdge(e);
            assert(e->refcnt >= -1);
            if (e->refcnt > 0)
                --e->refcnt;
            if (e->refcnt == 0)
                graph->release(e);
            ++cnt;
            if ((cnt % 1000) == 0)
                strategy->trivialNegation();
            if (root->done() || root->bad.id() != bddfalse.id())
                return root->good.id() == bddtrue.id();
        }

        if (root->done() || root->bad.id() != bddfalse.id())
            return root->good.id() == bddtrue.id();

        if (!strategy->trivialNegation()) {
            cnt = 0;
            strategy->releaseNegationEdges(strategy->maxDistance());
            continue;
        }
    }
    return root->good.id() == bddtrue.id();
}

void Algorithm::FCertainZeroFPA::checkEdge(Edge* e, bool only_assign) {
    if (e->handled)
        return;
    if (e->source->done()) {
        if (e->refcnt == 0)
            graph->release(e);
        return;
    }

#if DEBUG_DETAILED
    if (!only_assign) {
        std::cout << "### Checking edge: ";
        print_edge(e) << std::endl;
    }
#endif

    bool allOne = true;
    bool hasCZero = false;
    bdd good = bddtrue;
    bdd bad = bddfalse;
    // auto pre_empty = e->targets.empty();
    Configuration* lastUndecided = nullptr;
    {
        auto it = e->targets.begin();
        auto pit = e->targets.before_begin();
        while (it != e->targets.end()) {
            auto& [suc, feat] = *it;
            if (suc->assignment == ONE) {
                // x & tt == x, x | ff = x. hence erase
                assert(suc->good == bddtrue && suc->bad == bddfalse);
                e->targets.erase_after(pit);
                it = pit;
            } else if (suc->assignment == CZERO) {
                allOne = false;
                hasCZero = true;
                e->bad_iter = bddtrue;
                // assert(e->assignment == CZERO || only_assign);
                break;
            } else {
                good &= feat & suc->good;
                e->bad_iter |= bdd_imp(feat, suc->bad);
                allOne = false;
                if (lastUndecided == nullptr || *suc < *lastUndecided) {
                    lastUndecided = it->conf;
                }
            }
            pit = it;
            ++it;
        }
    }
    if (!hasCZero && !allOne) {
        for (auto* suc : e->source->successors) {
            bad &= suc->bad_iter;
        }
    }

    if (e->is_negated) {
        _processedNegationEdges += 1;
        // Process negation edge
        if (allOne) {
            e->handled = true;
            assert(e->refcnt > 0);
            if (only_assign)
                --e->refcnt;
            if (e->source->remove_suc(e, !only_assign)) {
                assert(e->source->successors.empty());
                finalAssign(e, CZERO);
            }
            if (e->refcnt == 0) {
                graph->release(e);
            }
        } else if (hasCZero) {
            finalAssign(e, ONE);
        } else {
            // assert(lastUndecided != nullptr);
            //  mixed assignment or not yet checked.
            if (only_assign)
                return;
            if (lastUndecided->assignment == ZERO && e->processed) {
                try_update(e->source, !good, good);
                finalAssign(e, DONE);
            } else {
                if (!e->processed) {
                    strategy->pushNegation(e);
                }
                lastUndecided->addDependency(e);
                if (lastUndecided->assignment == UNKNOWN) {
                    explore(lastUndecided);
                }
            }
        }
    } else {
        _processedEdges += 1;
        // Process hyper edge
        if (allOne) {
            finalAssign(e, ONE);
        } else if (hasCZero) {
            e->handled = true;
            assert(e->refcnt > 0);
            if (only_assign)
                --e->refcnt;
            if (e->source->remove_suc(e, !only_assign)) {
                finalAssign(e, CZERO);
            }
            if (e->refcnt == 0) {
                graph->release(e);
            }

        } else if (lastUndecided != nullptr) {
            if (only_assign)
                return;
            if (try_update(e->source, good, bad)) {
                push_dependencies(e->source);
            }
            if (!e->processed) {
                if (!lastUndecided->done()) {
                    for (auto& t : e->targets) {
                        t.conf->addDependency(e);
                    }
                }
            }
            if (lastUndecided->assignment == UNKNOWN) {
                explore(lastUndecided);
            }
        } else {
            // no undecided, no ONE or CZERO; we must be done with edge.
            if (try_update(e->source, good, bad))
                push_dependencies(e->source);
            e->source->remove_suc(e, !only_assign);
            if (e->source->nsuccs == 0) {
                finalAssign(e, DONE);
            }
        }
    }
    if (e->refcnt > 0 && !only_assign)
        e->processed = true;
    if (e->refcnt == 0)
        graph->release(e);
}

void Algorithm::FCertainZeroFPA::push_dependencies(const Configuration* c) {
    for (auto* e : c->dependency_set) {
        if (e->is_negated) {
            strategy->pushNegation(e);
        } else {
            strategy->pushDependency(e);
        }
    }
}

void Algorithm::FCertainZeroFPA::finalAssign(DependencyGraph::Edge* e,
                                             DependencyGraph::Assignment a) {
    finalAssign(e->source, a);
}

void Algorithm::FCertainZeroFPA::finalAssign(DependencyGraph::Configuration* c,
                                             DependencyGraph::Assignment a) {
    assert(a == ONE || a == CZERO || a == DONE);
    c->assignment = a;
    c->nsuccs = 0;

#if DEBUG_DETAILED
    std::cout << "### Assign: " << c->id << ", value: " << a << "\n.";
#endif

    if (a == CZERO) {
        assert(c->good == bddfalse);
        bool success = try_update(c, bddfalse, bddtrue);
        assert(success);
    } else if (a == ONE) {
        assert(c->bad == bddfalse);
        bool success = try_update(c, bddtrue, bddfalse);
        assert(success);
    }
    else {
        assert((c->bad | c->good) == bddtrue);
    }

    for (DependencyGraph::Edge* e : c->dependency_set) {
        if (!e->source->done()) {

            if (!e->is_negated || a == CZERO) {
                strategy->pushDependency(e);
            } else {
                strategy->pushNegation(e);
            }
        }
        assert(e->refcnt > 0);
        --e->refcnt;
        if (e->refcnt == 0)
            graph->release(e);
    }

    c->dependency_set.clear();
    for (auto* suc : c->successors) {
        --suc->refcnt;
        if (suc->refcnt == 0)
            graph->release(suc);
    }
    c->successors.clear();
}

bool Algorithm::FCertainZeroFPA::try_update(DependencyGraph::Configuration* c,
                                            bdd good, bdd bad) {
    assert((c->good >> good).id() == bddtrue.id() &&
           (c->bad >> bad).id() == bddtrue.id());

    bool assigned = false;
    if (c->good != good) {
        assert((c->good < good) == bddtrue);
        c->good = good;
        if (good == bddtrue)
            c->assignment = ONE;
        assigned = true;
    }
    if (c->bad != bad) {
        assert((c->bad < bad) == bddtrue);
        c->bad = bad;
        if (bad == bddtrue)
            c->assignment = CZERO;
        assigned = true;
    }

#if DEBUG_DETAILED
    if (assigned)
        std::cout << "### Assign: " << c->id << ", good: " << c->good.id() << " => "
                  << good.id() << "; bad: " << c->bad.id() << " => " << bad.id()
                  << std::endl;
    std::cerr << "### ASSIGN: [" << c->id
              << "]: " << to_string(static_cast<Assignment>(c->assignment))
              << "\n";

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
                std::cerr << "Error on :" << a << "\n\tGOT" << b << " expected "
                          << correct[a] << std::endl;
                assert(false);
            } else
                std::cerr << "### ¤ OK (" << c->id << ") = "
                          << DependencyGraph::to_string(
                                 (Assignment)c->assignment)
                          << std::endl;
        } else
            std::cerr << "### ¤ NO MATCH (" << c->id << ") "
                      << DependencyGraph::to_string((Assignment)c->assignment)
                      << std::endl;
    }
#endif
    return assigned;
}

void Algorithm::FCertainZeroFPA::explore(Configuration* c) {
    if (!c->successors.empty())
        return;
    c->seen_ = true;
    c->assignment = ZERO;

    {
        auto succs = graph->successors(c);
        c->nsuccs = succs.size();

#if DEBUG_DETAILED
        std::cout << "### Succs of " << c->id << ": \n###";
        for (auto suc : succs) {
            std::cout << "  ";
            print_edge(suc, std::cout) << "\n";
        }
#endif
        _exploredConfigurations += 1;
        _numberOfEdges += c->nsuccs;
        // before we start exploring, lets check if any of them determine
        // the outcome already!

        for (int32_t i = c->nsuccs - 1; i >= 0; --i) {
            checkEdge(succs[i], true);
            if (c->done()) {
                for (Edge* e : succs) {
                    assert(e->refcnt <= 1);
                    if (e->refcnt >= 1)
                        --e->refcnt;
                    if (e->refcnt == 0)
                        graph->release(e);
                }
                return;
            }
        }

        if (c->nsuccs == 0) {
            for (Edge* e : succs) {
                assert(e->refcnt <= 1);
                if (e->refcnt >= 1)
                    --e->refcnt;
                if (e->refcnt == 0)
                    graph->release(e);
            }
            finalAssign(c, CZERO);
            return;
        }

        for (Edge* succ : succs) {
            assert(succ->refcnt <= 1);
            if (succ->refcnt > 0) {
                strategy->pushEdge(succ);
                c->successors.push_front(succ);
                //--succ->refcnt;
                if (succ->refcnt == 0)
                    graph->release(succ);
            } else if (succ->refcnt == 0) {
                graph->release(succ);
            }
        }
        assert(c->nsuccs == c->num_successors());
    }
    strategy->flush();
}
} // namespace Featured
