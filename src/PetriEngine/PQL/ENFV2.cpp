/* Copyright (C) 2023  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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


#include "PetriEngine/PQL/ENFV2.h"
#include "utils/errors.h"
//#include "PetriEngine/PQL/ENFV2.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Evaluation.h"


// Macro to ensure that returns are done correctly
#ifndef NDEBUG
#define RETURN(x) { assert(x); return_value = x; has_returned = true; return;}
#else
#define RETURN(x) {return_value = x; return;}
#endif

namespace PetriEngine { namespace PQL {

        Condition_ptr
        ENFV2(Condition_ptr cond) {
            negstat_t s;
            EvaluationContext c(nullptr, nullptr);
            return ENFV2(cond, s, c, false, false, false);
        }

        Condition_ptr
        ENFV2(Condition_ptr cond, negstat_t &stats, const EvaluationContext &context, bool nested, bool negated, bool initrw) {
            ENFV2Visitor pn_visitor(stats, context, nested, negated, initrw);
            Visitor::visit(pn_visitor, cond);
            return pn_visitor.return_value;
        }

        void ENFV2Visitor::_accept(ControlCondition *element) {
            auto res = subvisit((*element)[0], false, negated);
            return_value = std::make_shared<ControlCondition>(res);
        }

        void ENFV2Visitor::_accept(EGCondition *element) {
            auto sub = subvisit(element->getCond(), true, false);
            if (negated) {
                RETURN(std::make_shared<NotCondition>(std::make_shared<EGCondition>(sub)))
            }
            else RETURN(std::make_shared<EGCondition>(sub))
        }

        void ENFV2Visitor::_accept(AGCondition *element) {
            ++stats[1];
            auto ef_cond = EFCondition(std::make_shared<NotCondition>(element->getCond()));
            RETURN(subvisit(&ef_cond, nested, !negated))
        }

        void ENFV2Visitor::_accept(EXCondition *element) {
            auto sub = subvisit(element->getCond(), true, false);
            if (negated) {
                RETURN(std::make_shared<NotCondition>(std::make_shared<EXCondition>(sub)))
            }
            else RETURN(std::make_shared<EXCondition>(sub))
        }

        void ENFV2Visitor::_accept(AXCondition *element) {
            ++stats[1];
            auto ex_cond = EXCondition(std::make_shared<NotCondition>(element->getCond()));
            RETURN(subvisit(&ex_cond, nested, !negated))
        }


        void ENFV2Visitor::_accept(EFCondition *element) {
            auto sub = subvisit(element->getCond(), true, false);
            if (negated) {
                RETURN(std::make_shared<NotCondition>(std::make_shared<EFCondition>(sub)))
            }
            else RETURN(std::make_shared<EFCondition>(sub))
        }

        void ENFV2Visitor::_accept(AFCondition *element) {
            ++stats[1];
            auto sub = subvisit(std::make_shared<NotCondition>(element->getCond()), true, false);
            // really we need EG condition here, but engine consistently computes least fixed-points
            // so this is a way to "trick" it to produce true from a component that did not violate
            // (should generate negation edge into AF query that we solve normally)

            auto af_cond = std::make_shared<EGCondition>(sub);
            if (!negated) {
                RETURN(std::make_shared<NotCondition>(std::make_shared<EGCondition>(sub)))
            }
            else RETURN(std::make_shared<EGCondition>(sub))
            //auto eg_cond = EGCondition(std::make_shared<NotCondition>(element->getCond()));
            //RETURN(std::make_shared<NotCondition>(af_cond))
        }

        void ENFV2Visitor::_accept(AUCondition *element) {
            // from baier & katoen, 2007:
            // A (p U q) === !E(!q U (!p & !q)) & !EG !q
            auto np = subvisit(std::make_shared<NotCondition>(element->getCond1()), true, false);
            auto nq = subvisit(std::make_shared<NotCondition>(element->getCond2()), true, false);
            auto q = subvisit(element->getCond2(), true, false);
            auto eu_cond = std::make_shared<AndCondition>(
                     std::make_shared<NotCondition>(std::make_shared<EUCondition>(nq, std::make_shared<AndCondition>(nq, np))),
                     std::make_shared<AFCondition>(q));
            if (negated) RETURN(std::make_shared<NotCondition>(eu_cond))
            else RETURN(eu_cond)
            /* RETURN(subvisit(negated ? std::make_shared<NotCondition>(eu_cond): eu_cond, nested, !negated))*/
        }


        void ENFV2Visitor::_accept(EUCondition *element) {
            auto p = subvisit(element->getCond1(), true, false);
            auto q = subvisit(element->getCond2(), true, false);
            auto eu_cond = std::make_shared<EUCondition>(p, q);
            if (negated) {
                RETURN(std::make_shared<NotCondition>(eu_cond))
            }
            else {
                RETURN(eu_cond)
            }
            //RETURN(subvisit(negated ? std::make_shared<NotCondition>(eu_cond) : eu_cond, nested, !negated))
        }

/*LTL negation push*/
        void ENFV2Visitor::_accept(UntilCondition* element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                auto b = subvisit(element->getCond2(), true, false);
                auto a = subvisit(element->getCond1(), true, false);

                if (auto cond = std::dynamic_pointer_cast<FCondition>(b)) {
                    static_assert(negstat_t::nrules >= 35);
                    ++stats[34];
                    if (negated)
                        return std::make_shared<NotCondition>(b);
                    return b;
                }

                auto c = std::make_shared<UntilCondition>(a, b);
                if (negated) return std::make_shared<NotCondition>(c);
                return c;
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(XCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                auto res = subvisit(element->getCond(), true, negated);
                if (res == BooleanCondition::TRUE_CONSTANT || res == BooleanCondition::FALSE_CONSTANT) {
                    return res;
                }
                return std::make_shared<XCondition>(res);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(FCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                auto a = subvisit(element->getCond(), true, false);
                if (!isTemporal(a)) {
                    auto res = std::make_shared<FCondition>(a);
                    if (negated) return std::make_shared<NotCondition>(res);
                    return res;
                }

                if (dynamic_cast<FCondition *>(a.get())) {
                    ++stats[31];
                    if (negated) a = std::make_shared<NotCondition>(a);
                    return a;
                } else if (auto cond = dynamic_cast<UntilCondition *>(a.get())) {
                    ++stats[32];
                    return subvisit(std::make_shared<FCondition>((*cond)[1]), nested, negated);
                } else if (auto cond = dynamic_cast<OrCondition *>(a.get())) {
                    if (!isTemporal(cond)) {
                        Condition_ptr b = std::make_shared<FCondition>(a);
                        if (negated) b = std::make_shared<NotCondition>(b);
                        return b;
                    }
                    ++stats[33];
                    std::vector<Condition_ptr> distributed;
                    for (auto &i: *cond) {
                        distributed.push_back(std::make_shared<FCondition>(i));
                    }
                    return subvisit(makeOr(distributed), nested, negated);
                } else {
                    Condition_ptr b = std::make_shared<FCondition>(a);
                    if (negated) b = std::make_shared<NotCondition>(b);
                    return b;
                }
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(ACondition *element) {
            auto e_cond = ECondition(std::make_shared<NotCondition>(element->getCond()));
            RETURN(subvisit(&e_cond, nested, !negated))
        }


        void ENFV2Visitor::_accept(ECondition *element) {
            // we forward the negated flag, we flip the outer quantifier later!
            auto _sub = subvisit(element->getCond(), nested, negated);
            if (negated) RETURN(std::make_shared<ACondition>(_sub))
            else RETURN(std::make_shared<ECondition>(_sub))
        }

        void ENFV2Visitor::_accept(GCondition *element) {
            auto f_cond = FCondition(std::make_shared<NotCondition>(element->getCond()));
            RETURN(subvisit(&f_cond, nested, !negated))
        }

/*Boolean connectives */
        Condition_ptr ENFV2Visitor::pushAnd(const std::vector<Condition_ptr> &_conds, bool _nested, bool negate_children) {
            std::vector<Condition_ptr> nef, other;
            for (auto &c: _conds) {
                auto n = subvisit(c, _nested, negate_children);
                if (n->isTriviallyFalse()) return n;
                if (n->isTriviallyTrue()) continue;
                if (auto neg = dynamic_cast<NotCondition *>(n.get())) {
                    if (auto ef = dynamic_cast<EFCondition *>((*neg)[0].get())) {
                        nef.push_back((*ef)[0]);
                    } else {
                        other.emplace_back(n);
                    }
                } else {
                    other.emplace_back(n);
                }
            }
            if (nef.size() + other.size() == 0)
                return BooleanCondition::TRUE_CONSTANT;
            if (nef.size() + other.size() == 1) {
                return nef.size() == 0 ?
                       other[0] :
                       std::make_shared<NotCondition>(std::make_shared<EFCondition>(nef[0]));
            }
            if (nef.size() != 0)
                other.push_back(
                        std::make_shared<NotCondition>(
                                std::make_shared<EFCondition>(
                                        makeOr(nef))));
            if (other.size() == 1) return other[0];
            auto res = makeAnd(other);
            return res;
        }

        Condition_ptr ENFV2Visitor::pushOr(const std::vector<Condition_ptr> &_conds, bool _nested, bool negate_children) {
            std::vector<Condition_ptr> nef, other;
            for (auto &c: _conds) {
                auto n = subvisit(c, _nested, negate_children);
                if (n->isTriviallyTrue()) {
                    return n;
                }
                if (n->isTriviallyFalse()) continue;
                if (auto ef = dynamic_cast<EFCondition *>(n.get())) {
                    nef.push_back((*ef)[0]);
                } else {
                    other.emplace_back(n);
                }
            }
            if (nef.size() + other.size() == 0)
                return BooleanCondition::FALSE_CONSTANT;
            if (nef.size() + other.size() == 1) {
                return nef.size() == 0 ? other[0] : std::make_shared<EFCondition>(nef[0]);
            }
            if (nef.size() != 0)
                other.push_back(
                        std::make_shared<EFCondition>(
                                makeOr(nef)));
            if (other.size() == 1) return other[0];
            return makeOr(other);
        }

        void ENFV2Visitor::_accept(OrCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                return negated ? pushAnd(element->getOperands(), nested, true) :
                       pushOr(element->getOperands(), nested, false);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }


        void ENFV2Visitor::_accept(AndCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                return negated ? pushOr(element->getOperands(), nested, true) :
                       pushAnd(element->getOperands(), nested, false);

            }, stats, context, nested, negated, initrw);
            RETURN(cond);
        }

        void ENFV2Visitor::_accept(CompareConjunction *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                return std::make_shared<CompareConjunction>(*element, negated);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }


        void ENFV2Visitor::_accept(NotCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                if (negated) ++stats[30];
                return subvisit(element->getCond(), nested, !negated);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        template<typename T> Condition_ptr
        ENFV2Visitor::pushFireableNegation(const shared_const_string &name, const Condition_ptr &compiled) {
            stats.negated_fireability = stats.negated_fireability || negated;
            if (compiled)
                return subvisit(compiled, nested, negated);
            if (negated)
                return std::make_shared<NotCondition>(std::make_shared<T>(name));
            else
                return std::make_shared<T>(name);
        }

        void ENFV2Visitor::_accept(UnfoldedFireableCondition *element) {
            RETURN(pushFireableNegation<UnfoldedFireableCondition>(element->getName(), element->getCompiled()))
        }

        void ENFV2Visitor::_accept(FireableCondition *element) {
            RETURN(pushFireableNegation<FireableCondition>(element->getName(), element->getCompiled()))
        }

        void ENFV2Visitor::_accept(LessThanCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                if (element->isTrivial())
                    return BooleanCondition::getShared(evaluate(element, context) xor negated);
                if (negated) return std::make_shared<LessThanOrEqualCondition>(element->getExpr2(), element->getExpr1());
                else return std::make_shared<LessThanCondition>(element->getExpr1(), element->getExpr2());
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(LessThanOrEqualCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                if (element->isTrivial())
                    return BooleanCondition::getShared(evaluate(element, context) xor negated);
                if (negated) return std::make_shared<LessThanCondition>(element->getExpr2(), element->getExpr1());
                else return std::make_shared<LessThanOrEqualCondition>(element->getExpr1(), element->getExpr2());
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        Condition_ptr ENFV2Visitor::pushEqual(CompareCondition *org, bool _negated, bool noteq) {
            if (org->isTrivial())
                return BooleanCondition::getShared(evaluate(org, context) xor _negated);
            for (auto i: {0, 1}) {
                if ((*org)[i]->placeFree() && evaluate((*org)[i].get(), context) == 0) {
                    if (_negated == noteq)
                        return std::make_shared<LessThanOrEqualCondition>((*org)[(i + 1) % 2],
                                                                          std::make_shared<LiteralExpr>(0));
                    else
                        return std::make_shared<LessThanOrEqualCondition>(std::make_shared<LiteralExpr>(1),
                                                                          (*org)[(i + 1) % 2]);
                }
            }
            if (_negated == noteq) return std::make_shared<EqualCondition>((*org)[0], (*org)[1]);
            else return std::make_shared<NotEqualCondition>((*org)[0], (*org)[1]);
        }

        void ENFV2Visitor::_accept(NotEqualCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                return pushEqual(element, negated, true);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }


        void ENFV2Visitor::_accept(EqualCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                return pushEqual(element, negated, false);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(BooleanCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                if (negated) return BooleanCondition::getShared(!element->value);
                else return BooleanCondition::getShared(element->value);
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(DeadlockCondition *element) {
            auto cond = initialMarkingRW([&]() -> Condition_ptr {
                if (negated) return std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK);
                else return DeadlockCondition::DEADLOCK;
            }, stats, context, nested, negated, initrw);
            RETURN(cond)
        }

        void ENFV2Visitor::_accept(UpperBoundsCondition* element) {
            if (negated) {
                throw base_error("UPPER BOUNDS CANNOT BE NEGATED!");
            }
            if(element->getCompiled())
                Visitor::visit(this, element->getCompiled());
            else
            RETURN(element->clone())
        }

        void ENFV2Visitor::_accept(UnfoldedUpperBoundsCondition *element) {
            if (negated) {
                throw base_error("UPPER BOUNDS CANNOT BE NEGATED!");
            }
            RETURN(std::make_shared<UnfoldedUpperBoundsCondition>(element->places(), element->getMax(), element->getOffset()));
        }

        void ENFV2Visitor::_accept(ShallowCondition *element) {
            if(element->getCompiled()) {
                RETURN(subvisit(element->getCompiled(), nested, negated))
            } else {
                if(negated) {
                    RETURN(std::static_pointer_cast<Condition>(std::make_shared<NotCondition>(element->clone())))
                } else {
                    RETURN(element->clone());
                }
            }
        }

        void ENFV2Visitor::_accept(KSafeCondition* element) {
            _accept(static_cast<ShallowCondition*>(element));
        }

        void ENFV2Visitor::_accept(LivenessCondition* element) {
            _accept(static_cast<ShallowCondition*>(element));
        }

        void ENFV2Visitor::_accept(QuasiLivenessCondition* element) {
            _accept(static_cast<ShallowCondition*>(element));
        }

        void ENFV2Visitor::_accept(StableMarkingCondition* element) {
            _accept(static_cast<ShallowCondition*>(element));
        }

        void ENFV2Visitor::_accept(PathSelectCondition* element) {
            RETURN(std::make_shared<PathSelectCondition>(element->name(), subvisit(element->child(), nested, negated), element->offset()))
        }

        Condition_ptr ENFV2Visitor::subvisit(Condition* condition, bool _nested, bool _negated) {
            {
                bool old_nested = nested;
                bool old_negated = negated;
                nested = _nested;
                negated = _negated;

                Visitor::visit(this, condition);
#ifndef NDEBUG
                assert(has_returned); // Subvisit should return value
                has_returned = false;
#endif

                nested = old_nested;
                negated = old_negated;

                return return_value;
            }
        }
    } }