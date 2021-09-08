/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 *                     Simon M. Virenfeldt <simon@simwir.dk>
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

#include "LTL/Stubborn/EvalAndSetVisitor.h"

namespace LTL {

using namespace PetriEngine::PQL;

void EvalAndSetVisitor::_accept(PetriEngine::PQL::ACondition *condition) {
    condition->getCond()->visit(*this);
}

void EvalAndSetVisitor::_accept(PetriEngine::PQL::ECondition *condition) {
    condition->getCond()->visit(*this);
}

void EvalAndSetVisitor::_accept(PetriEngine::PQL::XCondition *condition) {
    condition->getCond()->visit(*this);
}

void EvalAndSetVisitor::_accept(PetriEngine::PQL::GCondition *condition) {
    condition->getCond()->visit(*this);
    auto res = condition->getCond()->get_satisfied();
    if (res != Condition::RFALSE)
        res = Condition::RUNKNOWN;
    condition->set_satisfied(res);
}

void EvalAndSetVisitor::_accept(PetriEngine::PQL::FCondition *condition) {
    condition->getCond()->visit(*this);
    auto res = condition->getCond()->get_satisfied();
    if (res != Condition::RTRUE)
        res = Condition::RUNKNOWN;
    condition->set_satisfied(res);
}

void EvalAndSetVisitor::_accept(PetriEngine::PQL::UntilCondition *condition) {
    condition->get_cond1()->visit(*this);
    condition->get_cond2()->visit(*this);
    auto r2 = condition->get_cond2()->get_satisfied();
    if (r2 != Condition::RFALSE) {
        condition->set_satisfied(r2);
        return;
    }
    auto r1 = condition->get_cond1()->get_satisfied();
    if (r1 == Condition::RFALSE) {
        condition->set_satisfied(Condition::RFALSE);
        return;
    }
    condition->set_satisfied(Condition::RUNKNOWN);
}

void EvalAndSetVisitor::_accept(NotCondition *element) {
    element->getCond()->visit(*this);
    auto res = element->getCond()->get_satisfied();
    if (res != Condition::RUNKNOWN)
        res = res == Condition::RFALSE ? Condition::RTRUE : Condition::RFALSE;
    element->set_satisfied(res);
}

void EvalAndSetVisitor::_accept(AndCondition *element) {
    auto res = Condition::RTRUE;
    for (auto &c : *element) {
        c->visit(*this);
        auto r = c->get_satisfied();
        if (r == Condition::RFALSE)
            res = Condition::RFALSE;
        else if (r == Condition::RUNKNOWN && res != Condition::RFALSE)
            res = Condition::RUNKNOWN;
    }
    element->set_satisfied(res);
}

void EvalAndSetVisitor::_accept(OrCondition *element) {
    auto res = Condition::RFALSE;
    for (auto &c : *element) {
        c->visit(*this);
        auto r = c->get_satisfied();
        if (r == Condition::RTRUE)
            res = Condition::RTRUE;
        else if (r == Condition::RUNKNOWN && res != Condition::RTRUE)
            res = Condition::RUNKNOWN;
    }
    element->set_satisfied(res);
}

void EvalAndSetVisitor::_accept(LessThanCondition *element) { element->eval_and_set(_context); }

void EvalAndSetVisitor::_accept(LessThanOrEqualCondition *element) {
    element->eval_and_set(_context);
}

void EvalAndSetVisitor::_accept(EqualCondition *element) { element->eval_and_set(_context); }

void EvalAndSetVisitor::_accept(NotEqualCondition *element) { element->eval_and_set(_context); }

void EvalAndSetVisitor::_accept(DeadlockCondition *element) { element->eval_and_set(_context); }

void EvalAndSetVisitor::_accept(CompareConjunction *element) { element->eval_and_set(_context); }

void EvalAndSetVisitor::_accept(UnfoldedUpperBoundsCondition *element) {
    element->eval_and_set(_context);
}

void EvalAndSetVisitor::_accept(BooleanCondition *element) { element->eval_and_set(_context); }
} // namespace LTL
