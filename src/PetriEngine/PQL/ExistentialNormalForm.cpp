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


#include "PetriEngine/PQL/ExistentialNormalForm.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Evaluation.h"

#include "utils/errors.h"

// Macro to ensure that returns are done correctly
#ifndef NDEBUG
#define RETURN(x) { assert(x); return_value = x; has_returned = true; return;}
#else
#define RETURN(x) {return_value = x; return;}
#endif


namespace PetriEngine::PQL {

    Condition_ptr ctl_to_enf(Condition_ptr cond)
    {
        ExistentialNormalForm visitor;
        Visitor::visit(visitor, cond);
        return visitor.return_value;
    }

    void ExistentialNormalForm::_accept(NotCondition* element) {
        auto sub = subvisit(element->getCond().get(), !_negated);
        RETURN(sub)
    }

    Condition_ptr ExistentialNormalForm::push_and(const std::vector<Condition_ptr> &_conds, bool negate_children) {
        std::vector<Condition_ptr> nef, other;
        for (auto &c: _conds) {
            auto n = subvisit(c, negate_children);
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

    Condition_ptr ExistentialNormalForm::push_or(const std::vector<Condition_ptr> &_conds, bool negate_children) {
        std::vector<Condition_ptr> nef, other;
        for (auto &c: _conds) {
            auto n = subvisit(c, negate_children);
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

    void ExistentialNormalForm::_accept(AndCondition* element) {
        RETURN(_negated ? push_or(element->getOperands(), true) : push_and(element->getOperands(), false));
    }

    void ExistentialNormalForm::_accept(OrCondition* element) {
        RETURN(_negated ? push_and(element->getOperands(), true) : push_or(element->getOperands(), false));
    }

    void ExistentialNormalForm::_accept(LessThanCondition* element) {
        if (_negated) {
            RETURN(std::make_shared<LessThanOrEqualCondition>(element->getExpr2(), element->getExpr1()))
        }
        else
            RETURN(std::make_shared<LessThanCondition>(element->getExpr1(), element->getExpr2()))
    }

    void ExistentialNormalForm::_accept(LessThanOrEqualCondition* element) {
        if (_negated) {
            RETURN(std::make_shared<LessThanCondition>(element->getExpr2(), element->getExpr1()))
        }
        else
        RETURN(std::make_shared<LessThanOrEqualCondition>(element->getExpr1(), element->getExpr2()))
    }

    void ExistentialNormalForm::_accept(EqualCondition* element) {
        if (_negated) {
            RETURN(std::make_shared<NotEqualCondition>(element->getExpr1(), element->getExpr2()))
        }
        else
            RETURN(std::make_shared<EqualCondition>(element->getExpr1(), element->getExpr2()))
    }

    void ExistentialNormalForm::_accept(NotEqualCondition* element) {
        if (_negated) {
            RETURN(std::make_shared<EqualCondition>(element->getExpr1(), element->getExpr2()))
        }
        else
        RETURN(std::make_shared<NotEqualCondition>(element->getExpr1(), element->getExpr2()))
    }

    void ExistentialNormalForm::_accept(DeadlockCondition* element) {
        if (_negated) RETURN(std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK))
        else RETURN(DeadlockCondition::DEADLOCK);
    }

    void ExistentialNormalForm::_accept(CompareConjunction* element) {
        RETURN(std::make_shared<CompareConjunction>(*element, _negated))
    }

    void ExistentialNormalForm::_accept(UnfoldedUpperBoundsCondition* element) {
        RETURN(std::make_shared<UnfoldedUpperBoundsCondition>(*element))

    }

    void ExistentialNormalForm::_accept(EFCondition* condition) {
        auto sub = subvisit(condition->getCond().get(), _negated);
        RETURN(std::make_shared<EFCondition>(sub))
    }

    void ExistentialNormalForm::_accept(EGCondition* condition) {
        auto sub = subvisit(condition->getCond().get(), _negated);
        RETURN(std::make_shared<EGCondition>(sub))
    }

    void ExistentialNormalForm::_accept(AGCondition* condition) {
        auto ef = std::make_shared<EFCondition>(std::make_shared<NotCondition>(condition->getCond()));
        RETURN(std::make_shared<NotCondition>(subvisit(ef, _negated)))
    }

    void ExistentialNormalForm::_accept(AFCondition* condition) {
        auto eg = std::make_shared<EGCondition>(condition->getCond());
        //auto eg = std::make_shared<EGCondition>(std::make_shared<NotCondition>(condition->getCond()));
        RETURN(std::make_shared<NotCondition>(subvisit(eg, _negated)))
    }

    void ExistentialNormalForm::_accept(EXCondition* condition) {
        auto sub = subvisit(condition->getCond().get(), _negated);
        RETURN(std::make_shared<EXCondition>(sub))
    }

    void ExistentialNormalForm::_accept(AXCondition* condition) {
        auto ex = std::make_shared<EXCondition>(std::make_shared<NotCondition>(condition->getCond()));
        RETURN(std::make_shared<NotCondition>(subvisit(ex, _negated)))
    }

    void ExistentialNormalForm::_accept(EUCondition* condition) {
        auto p = subvisit(condition->getCond1().get(), false);
        auto q = subvisit(condition->getCond2().get(), false);
        RETURN(std::make_shared<EUCondition>(p, q))
    }

    void ExistentialNormalForm::_accept(AUCondition* condition) {
        // from baier & katoen, 2007:
        // A (p U q) === !E(!q U (!p & !q)) & !EG !q
        auto notp = std::make_shared<NotCondition>(condition->getCond1());
        auto notq = std::make_shared<NotCondition>(condition->getCond2());
        auto np = subvisit(notp, false);
        auto nq = subvisit(notq, false);
        /*auto p = subvisit(condition->getCond1().get(), true);
        auto q = subvisit(condition->getCond2().get(), true);
        auto np = std::make_shared<NotCondition>(p);
        auto nq = std::make_shared<NotCondition>(q);*/
        RETURN(std::make_shared<AndCondition>(
                 std::make_shared<NotCondition>(
                         std::make_shared<EUCondition>(nq,
                                                       std::make_shared<AndCondition>(nq, np))),
                 std::make_shared<NotCondition>(std::make_shared<EGCondition>(nq))))
    }

    Condition_ptr ExistentialNormalForm::subvisit(Condition* cond, bool negated) {
        bool old = _negated;
        _negated = negated;
        Visitor::visit(this, cond);
#ifndef NDEBUG
        assert(has_returned); // Subvisit should return value
        has_returned = false;
#endif
        _negated = old;

        return return_value;
    }

    void ExistentialNormalForm::_accept(BooleanCondition* element) {
        RETURN(_negated ? BooleanCondition::getShared(!element->value) : BooleanCondition::getShared(element->value))
    }
}