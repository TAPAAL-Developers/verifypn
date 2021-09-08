#include "CTL/PetriNets/OnTheFlyDG.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <limits>
#include <queue>

#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/Structures/StateSet.h"
#include "PetriEngine/Stubborn/ReachabilityStubbornSet.h"
#include "PetriEngine/SuccessorGenerator.h"

using namespace PetriEngine::PQL;
using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(const PetriEngine::PetriNet &t_net, bool partial_order)
    : _net(t_net), _encoder(t_net.number_of_places()),
      _edge_alloc(new linked_bucket_t<DependencyGraph::Edge, size_t(1024) * size_t(10)>(1)),
      _conf_alloc(new linked_bucket_t<char[sizeof(PetriConfig)], size_t(1024) * size_t(1024)>(1)),
      _redgen(t_net, std::make_shared<PetriEngine::ReachabilityStubbornSet>(t_net)),
      _partial_order(partial_order) {}

OnTheFlyDG::~OnTheFlyDG() {
    cleanup();
    // Note: initial marking is in the markings set, therefore it will be deleted by the for loop
    // TODO: Ensure we don't leak memory here, when code moving is done
    size_t s = _conf_alloc->size();
    for (size_t i = 0; i < s; ++i) {
        ((PetriConfig *)&(*_conf_alloc)[i])->~PetriConfig();
    }
    delete _conf_alloc;
    delete _edge_alloc;
}

auto OnTheFlyDG::initial_eval() -> Condition::result_e {
    initial_configuration();
    EvaluationContext e(_query_marking.marking(), &_net);
    return _query->evaluate(e);
}

auto OnTheFlyDG::fast_eval(Condition *query, Marking *unfolded) -> Condition::result_e {
    EvaluationContext e(unfolded->marking(), &_net);
    return query->evaluate(e);
}

auto OnTheFlyDG::successors(Configuration *c) -> std::vector<DependencyGraph::Edge *> {
    PetriEngine::PQL::DistanceContext context(_net, _query_marking.marking());
    PetriConfig *v = static_cast<PetriConfig *>(c);
    _trie.unpack(v->_marking, _encoder.scratchpad().raw());
    _encoder.decode(_query_marking.marking(), _encoder.scratchpad().raw());
    //    v->printConfiguration();
    std::vector<Edge *> succs;
    auto query_type = v->_query->get_query_type();
    if (query_type == EVAL) {
        assert(false);
        // assert(false && "Someone told me, this was a bad place to be.");
        if (fast_eval(_query, &_query_marking) == Condition::RTRUE) {
            succs.push_back(new_edge(*v, 0)); ///*v->query->distance(context))*/0);
        }
    } else if (query_type == LOPERATOR) {
        if (v->_query->get_quantifier() == NEG) {
            // no need to try to evaluate here -- this is already transient in other evaluations.
            auto cond = static_cast<NotCondition *>(v->_query);
            Configuration *c = create_configuration(v->_marking, (*cond)[0]);
            Edge *e = new_edge(*v, /*v->query->distance(context)*/ 0);
            e->_is_negated = true;
            e->add_target(c);
            succs.push_back(e);
        } else if (v->_query->get_quantifier() == AND) {
            auto cond = static_cast<AndCondition *>(v->_query);
            // Check if left is false
            std::vector<Condition *> conds;
            for (auto &c : *cond) {
                auto res = fast_eval(c.get(), &_query_marking);
                if (res == Condition::RFALSE) {
                    return succs;
                }
                if (res == Condition::RUNKNOWN) {
                    conds.push_back(c.get());
                }
            }

            Edge *e = new_edge(*v, /*cond->distance(context)*/ 0);

            // If we get here, then either both propositions are true (shouldn't be possible)
            // Or a temporal operator and a true proposition
            // Or both are temporal
            for (auto c : conds) {
                assert(c->is_temporal());
                e->add_target(create_configuration(v->_marking, c));
            }
            succs.push_back(e);
        } else if (v->_query->get_quantifier() == OR) {
            auto cond = static_cast<OrCondition *>(v->_query);
            // Check if left is true
            std::vector<Condition *> conds;
            for (auto &c : *cond) {
                auto res = fast_eval(c.get(), &_query_marking);
                if (res == Condition::RTRUE) {
                    succs.push_back(new_edge(*v, 0));
                    return succs;
                }
                if (res == Condition::RUNKNOWN) {
                    conds.push_back(c.get());
                }
            }

            // If we get here, either both propositions are false
            // Or one is false and one is temporal
            // Or both temporal
            for (auto c : conds) {
                assert(c->is_temporal());
                Edge *e = new_edge(*v, /*cond->distance(context)*/ 0);
                e->add_target(create_configuration(v->_marking, c));
                succs.push_back(e);
            }
        } else {
            assert(false &&
                   "An unknown error occoured in the loperator-part of the successor generator");
        }
    } else if (query_type == PATHQEURY) {
        if (v->_query->get_quantifier() == A) {
            if (v->_query->get_path() == U) {
                auto cond = static_cast<AUCondition *>(v->_query);
                Edge *right = nullptr;
                auto r1 = fast_eval((*cond)[1], &_query_marking);
                if (r1 != Condition::RUNKNOWN) {
                    // right side is not temporal, eval it right now!
                    if (r1 == Condition::RTRUE) { // satisfied, no need to go through successors
                        succs.push_back(new_edge(*v, 0));
                        return succs;
                    } // else: It's not valid, no need to add any edge, just add successors
                } else {
                    // right side is temporal, we need to evaluate it as normal
                    Configuration *c = create_configuration(v->_marking, (*cond)[1]);
                    right = new_edge(*v, /*(*cond)[1]->distance(context)*/ 0);
                    right->add_target(c);
                }
                bool valid = false;
                Configuration *left = nullptr;
                auto r0 = fast_eval((*cond)[0], &_query_marking);
                if (r0 != Condition::RUNKNOWN) {
                    // left side is not temporal, eval it right now!
                    valid = r0 == Condition::RTRUE;
                } else {
                    // left side is temporal, include it in the edge
                    left = create_configuration(v->_marking, (*cond)[0]);
                }
                if (valid || left != nullptr) {
                    // if left side is guaranteed to be not satisfied, skip successor generation
                    Edge *leftEdge = nullptr;
                    next_states(
                        _query_marking, cond,
                        [&]() { leftEdge = new_edge(*v, std::numeric_limits<uint32_t>::max()); },
                        [&](Marking &mark) {
                            auto res = fast_eval(cond, &mark);
                            if (res == Condition::RTRUE)
                                return true;
                            if (res == Condition::RFALSE) {
                                left = nullptr;
                                leftEdge->_targets.clear();
                                leftEdge = nullptr;
                                return false;
                            }
                            context.set_marking(mark.marking());
                            Configuration *c = create_configuration(create_marking(mark), cond);
                            leftEdge->add_target(c);
                            return true;
                        },
                        [&]() {
                            if (leftEdge) {
                                if (left != nullptr) {
                                    leftEdge->add_target(left);
                                }
                                succs.push_back(leftEdge);
                            }
                        });
                } // else: Left side is not temporal and it's false, no way to succeed there...

                if (right != nullptr) {
                    succs.push_back(right);
                }
            } else if (v->_query->get_path() == F) {
                auto cond = static_cast<AFCondition *>(v->_query);
                Edge *subquery = nullptr;
                auto r = fast_eval((*cond)[0], &_query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(new_edge(*v, 0));
                        return succs;
                    }
                } else {
                    subquery = new_edge(*v, /*cond->distance(context)*/ 0);
                    Configuration *c = create_configuration(v->_marking, (*cond)[0]);
                    subquery->add_target(c);
                }
                Edge *e1 = nullptr;
                next_states(
                    _query_marking, cond,
                    [&]() { e1 = new_edge(*v, std::numeric_limits<uint32_t>::max()); },
                    [&](Marking &mark) {
                        auto res = fast_eval(cond, &mark);
                        if (res == Condition::RTRUE)
                            return true;
                        if (res == Condition::RFALSE) {
                            if (subquery) {
                                --subquery->_refcnt;
                                release(subquery);
                                subquery = nullptr;
                            }
                            e1->_targets.clear();
                            return false;
                        }
                        context.set_marking(mark.marking());
                        Configuration *c = create_configuration(create_marking(mark), cond);
                        e1->add_target(c);
                        return true;
                    },
                    [&]() { succs.push_back(e1); });

                if (subquery != nullptr) {
                    succs.push_back(subquery);
                }
            } else if (v->_query->get_path() == X) {
                auto cond = static_cast<AXCondition *>(v->_query);
                Edge *e = new_edge(*v, std::numeric_limits<uint32_t>::max());
                Condition::result_e allValid = Condition::RTRUE;
                next_states(
                    _query_marking, cond, []() {},
                    [&](Marking &mark) {
                        auto res = fast_eval((*cond)[0], &mark);
                        if (res != Condition::RUNKNOWN) {
                            if (res == Condition::RFALSE) {
                                allValid = Condition::RFALSE;
                                return false;
                            }
                        } else {
                            allValid = Condition::RUNKNOWN;
                            context.set_marking(mark.marking());
                            Configuration *c =
                                create_configuration(create_marking(mark), (*cond)[0]);
                            e->add_target(c);
                        }
                        return true;
                    },
                    []() {});
                if (allValid == Condition::RUNKNOWN) {
                    succs.push_back(e);
                } else if (allValid == Condition::RTRUE) {
                    e->_targets.clear();
                    succs.push_back(e);
                } else {
                }
            } else if (v->_query->get_path() == G) {
                assert(false &&
                       "Path operator G had not been translated - Parse error detected in succ()");
            } else
                assert(false && "An unknown error occoured in the successor generator");
        } else if (v->_query->get_quantifier() == E) {
            if (v->_query->get_path() == U) {
                auto cond = static_cast<EUCondition *>(v->_query);
                Edge *right = nullptr;
                auto r1 = fast_eval((*cond)[1], &_query_marking);
                if (r1 == Condition::RUNKNOWN) {
                    Configuration *c = create_configuration(v->_marking, (*cond)[1]);
                    right = new_edge(*v, /*(*cond)[1]->distance(context)*/ 0);
                    right->add_target(c);
                } else {
                    bool valid = r1 == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(new_edge(*v, 0));
                        return succs;
                    } // else: right condition is not satisfied, no need to add an edge
                }

                Configuration *left = nullptr;
                bool valid = false;
                next_states(
                    _query_marking, cond,
                    [&]() {
                        auto r0 = fast_eval((*cond)[0], &_query_marking);
                        if (r0 == Condition::RUNKNOWN) {
                            left = create_configuration(v->_marking, (*cond)[0]);
                        } else {
                            valid = r0 == Condition::RTRUE;
                        }
                    },
                    [&](Marking &marking) {
                        if (left == nullptr && !valid)
                            return false;
                        auto res = fast_eval(cond, &marking);
                        if (res == Condition::RFALSE)
                            return true;
                        if (res == Condition::RTRUE) {
                            for (auto s : succs) {
                                --s->_refcnt;
                                release(s);
                            }
                            succs.clear();
                            succs.push_back(new_edge(*v, 0));
                            if (right) {
                                --right->_refcnt;
                                release(right);
                                right = nullptr;
                            }

                            if (left) {
                                succs.back()->add_target(left);
                            }

                            return false;
                        }
                        context.set_marking(marking.marking());
                        Edge *e = new_edge(*v, /*cond->distance(context)*/ 0);
                        Configuration *c1 = create_configuration(create_marking(marking), cond);
                        e->add_target(c1);
                        if (left != nullptr) {
                            e->add_target(left);
                        }
                        succs.push_back(e);
                        return true;
                    },
                    []() {});

                if (right != nullptr) {
                    succs.push_back(right);
                }
            } else if (v->_query->get_path() == F) {
                auto cond = static_cast<EFCondition *>(v->_query);
                Edge *subquery = nullptr;
                auto r = fast_eval((*cond)[0], &_query_marking);
                if (r != Condition::RUNKNOWN) {
                    bool valid = r == Condition::RTRUE;
                    if (valid) {
                        succs.push_back(new_edge(*v, 0));
                        return succs;
                    }
                } else {
                    Configuration *c = create_configuration(v->_marking, (*cond)[0]);
                    subquery = new_edge(*v, /*cond->distance(context)*/ 0);
                    subquery->add_target(c);
                }

                next_states(
                    _query_marking, cond, []() {},
                    [&](Marking &mark) {
                        auto res = fast_eval(cond, &mark);
                        if (res == Condition::RFALSE)
                            return true;
                        if (res == Condition::RTRUE) {
                            for (auto s : succs) {
                                --s->_refcnt;
                                release(s);
                            }
                            succs.clear();
                            succs.push_back(new_edge(*v, 0));
                            if (subquery) {
                                --subquery->_refcnt;
                                release(subquery);
                            }
                            subquery = nullptr;
                            return false;
                        }
                        context.set_marking(mark.marking());
                        Edge *e = new_edge(*v, /*cond->distance(context)*/ 0);
                        Configuration *c = create_configuration(create_marking(mark), cond);
                        e->add_target(c);
                        succs.push_back(e);
                        return true;
                    },
                    []() {});

                if (subquery != nullptr) {
                    succs.push_back(subquery);
                }
            } else if (v->_query->get_path() == X) {
                auto cond = static_cast<EXCondition *>(v->_query);
                auto query = (*cond)[0];
                next_states(
                    _query_marking, cond, []() {},
                    [&](Marking &marking) {
                        auto res = fast_eval(query, &marking);
                        if (res == Condition::RTRUE) {
                            for (auto s : succs) {
                                --s->_refcnt;
                                release(s);
                            }
                            succs.clear();
                            succs.push_back(new_edge(*v, 0));
                            return false;
                        } // else: It can't hold there, no need to create an edge
                        else if (res == Condition::RUNKNOWN) {
                            context.set_marking(marking.marking());
                            Edge *e = new_edge(*v, /*(*cond)[0]->distance(context)*/ 0);
                            Configuration *c = create_configuration(create_marking(marking), query);
                            e->add_target(c);
                            succs.push_back(e);
                        }
                        return true;
                    },
                    []() {});
            } else if (v->_query->get_path() == G) {
                assert(false &&
                       "Path operator G had not been translated - Parse error detected in succ()");
            } else
                assert(false && "An unknown error occoured in the successor generator");
        }
    } else {
        assert(false && "Should never happen");
    }
    /*
    // legacy code from distributed version
    if(succs.size() == 1 && succs[0]->targets.size() == 1)
    {
        ((PetriConfig*)succs[0]->targets[0])->setOwner(v->getOwner());
    }*/
    return succs;
}

auto OnTheFlyDG::initial_configuration() -> Configuration * {
    if (_working_marking.marking() == nullptr) {
        _working_marking.set_marking(_net.make_initial_marking());
        _query_marking.set_marking(_net.make_initial_marking());
        _initial_config = create_configuration(create_marking(_working_marking), this->_query);
    }
    return _initial_config;
}

void OnTheFlyDG::next_states(Marking &t_marking, Condition *ptr, std::function<void()> &&pre,
                             std::function<bool(Marking &)> &&foreach,
                             std::function<void()> &&post) {
    bool first = true;
    memcpy(_working_marking.marking(), _query_marking.marking(),
           _net.number_of_places() * sizeof(PetriEngine::MarkVal));
    auto qf = static_cast<QuantifierCondition *>(ptr);
    if (!_partial_order || ptr->get_quantifier() != E || ptr->get_path() != F ||
        (*qf)[0]->is_temporal()) {
        PetriEngine::SuccessorGenerator PNGen(_net);
        do_work<PetriEngine::SuccessorGenerator>(PNGen, first, std::move(pre), std::move(foreach));
    } else {
        _redgen.set_query(ptr);
        do_work<PetriEngine::ReducingSuccessorGenerator>(_redgen, first, std::move(pre),
                                                         std::move(foreach));
    }

    if (!first)
        post();
}

void OnTheFlyDG::cleanup() {
    while (!_recycle.empty()) {
        assert(_recycle.top()->_refcnt == -1);
        _recycle.pop();
    }
    // TODO, implement proper cleanup
}

void OnTheFlyDG::set_query(const Condition_ptr &query) {
    this->_query = query;
    delete[] _working_marking.marking();
    delete[] _query_marking.marking();
    _working_marking.set_marking(nullptr);
    _query_marking.set_marking(nullptr);
    initial_configuration();
    assert(this->_query);
}

auto OnTheFlyDG::configuration_count() const -> size_t { return _configurationCount; }

auto OnTheFlyDG::marking_count() const -> size_t { return _markingCount; }

auto OnTheFlyDG::create_configuration(size_t marking, Condition *t_query) -> PetriConfig * {
    auto &configs = _trie.get_data(marking);
    for (PetriConfig *c : configs) {
        if (c->_query == t_query)
            return c;
    }

    _configurationCount++;
    size_t id = _conf_alloc->next(0);
    char *mem = (*_conf_alloc)[id];
    PetriConfig *newConfig = new (mem) PetriConfig();
    newConfig->_marking = marking;
    newConfig->_query = t_query;
    configs.push_back(newConfig);
    return newConfig;
}

auto OnTheFlyDG::create_marking(Marking &t_marking) -> size_t {
    uint32_t sum = 0;
    bool allsame = true;
    uint32_t val = 0;
    uint32_t active = 0;
    uint32_t last = 0;
    PetriEngine::Structures::StateSetInterface::marking_stats(
        t_marking.marking(), sum, allsame, val, active, last, _net.number_of_places());
    unsigned char type = _encoder.get_type(sum, active, allsame, val);
    size_t length = _encoder.encode(t_marking.marking(), type);
    BinaryWrapper w = BinaryWrapper(_encoder.scratchpad().raw(), length * 8);
    auto tit = _trie.insert(w.raw(), w.size());
    if (tit.first) {
        _markingCount++;
    }

    return tit.second;
}

void OnTheFlyDG::release(Edge *e) {
    assert(e->_refcnt == 0);
    e->_is_negated = false;
    e->_processed = false;
    e->_source = nullptr;
    e->_targets.clear();
    e->_refcnt = -1;
    e->_handled = false;
    _recycle.push(e);
}

auto OnTheFlyDG::new_edge(Configuration &t_source, uint32_t weight) -> Edge * {
    Edge *e = nullptr;
    if (_recycle.empty()) {
        size_t n = _edge_alloc->next(0);
        e = &(*_edge_alloc)[n];
    } else {
        e = _recycle.top();
        e->_refcnt = 0;
        _recycle.pop();
    }
    assert(e->_targets.empty());
    /*e->assignment = UNKNOWN;
    e->children = 0;*/
    e->_source = &t_source;
    assert(e->_refcnt == 0);
    ++e->_refcnt;
    return e;
}

} // namespace PetriNets
