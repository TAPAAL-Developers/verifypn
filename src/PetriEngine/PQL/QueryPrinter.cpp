/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#include "PetriEngine/PQL/QueryPrinter.h"

namespace PetriEngine::PQL {
void QueryPrinter::accept(const NotCondition *element) {
    _os << "(not ";
    element->operator[](0)->visit(*this);
    _os << ")";
}

void QueryPrinter::accept(const LogicalCondition *element, const std::string &op) {
    _os << "(";
    (*element)[0]->visit(*this);
    for (size_t i = 1; i < element->operands(); ++i) {
        _os << " " << op << " ";
        (*element)[i]->visit(*this);
    }
    _os << ")";
}

void QueryPrinter::accept(const CompareCondition *element, const std::string &op) {
    _os << "(";
    (*element)[0]->visit(*this);
    _os << " " << op << " ";
    (*element)[1]->visit(*this);
    _os << ")";
}

void QueryPrinter::accept(const AndCondition *element) { accept(element, "and"); }

void QueryPrinter::accept(const OrCondition *element) { accept(element, "or"); }

void QueryPrinter::accept(const LessThanCondition *element) { accept(element, "<"); }

void QueryPrinter::accept(const LessThanOrEqualCondition *element) { accept(element, "<="); }

void QueryPrinter::accept(const EqualCondition *element) { accept(element, "=="); }

void QueryPrinter::accept(const NotEqualCondition *element) { accept(element, "!="); }

void QueryPrinter::accept(const DeadlockCondition *element) { _os << "deadlock"; }

void QueryPrinter::accept(const CompareConjunction *element) {
    _os << "(";
    if (element->is_negated())
        _os << "not";
    bool first = true;
    for (const auto &cons : *element) {
        if (!first)
            _os << " and ";
        if (cons._lower != 0)
            _os << "(" << cons._lower << " <= " << cons._name << ")";
        if (cons._lower != 0 && cons._upper != std::numeric_limits<uint32_t>::max())
            _os << " and ";
        if (cons._upper != std::numeric_limits<uint32_t>::max())
            _os << "(" << cons._upper << " >= " << cons._name << ")";
        first = false;
    }
    _os << ")";
}

void QueryPrinter::accept(const UnfoldedUpperBoundsCondition *element) {
    _os << "bounds (";
    auto places = element->places();
    for (size_t i = 0; i < places.size(); ++i) {
        if (i != 0)
            _os << ", ";
        _os << places[i]._name;
    }
    _os << ")";
}

void QueryPrinter::accept(const EFCondition *condition) {
    _os << "EF ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const EGCondition *condition) {
    _os << "EG ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const AGCondition *condition) {
    _os << "AG ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const AFCondition *condition) {
    _os << "AF ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const EXCondition *condition) {
    _os << "AF ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const AXCondition *condition) {
    _os << "AX ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const EUCondition *condition) {
    _os << "E ";
    accept((UntilCondition *)condition);
}

void QueryPrinter::accept(const AUCondition *condition) {
    _os << "A ";
    accept((UntilCondition *)condition);
}

void QueryPrinter::accept(const ACondition *condition) {
    _os << "A ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const ECondition *condition) {
    _os << "E ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const GCondition *condition) {
    _os << "G ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const FCondition *condition) {
    _os << "F ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const XCondition *condition) {
    _os << "X ";
    (*condition)[0]->visit(*this);
}

void QueryPrinter::accept(const UntilCondition *condition) {
    _os << "(";
    condition->get_cond1()->visit(*this);
    _os << " U ";
    condition->get_cond2()->visit(*this);
    _os << ")";
}

void QueryPrinter::accept(const UnfoldedFireableCondition *element) {
    _os << "is-fireable(" << element->get_name() << ")";
}

void QueryPrinter::accept(const FireableCondition *element) {
    _os << "is-fireable(" << element->get_name() << ")";
}

void QueryPrinter::accept(const UpperBoundsCondition *element) {
    _os << "bounds (";
    auto places = element->get_places();
    for (size_t i = 0; i < places.size(); ++i) {
        if (i != 0)
            _os << ", ";
        _os << places[i];
    }
    _os << ")";
}

void QueryPrinter::accept(const LivenessCondition *element) {
    const Condition_ptr &cond = element->get_compiled();
    if (cond) {
        cond->visit(*this);
    } else {
        _os << "liveness";
    }
}

void QueryPrinter::accept(const KSafeCondition *element) {
    if (element->get_compiled()) {
        element->get_compiled()->visit(*this);
    } else {
        _os << "k-safe(";
        element->get_bound()->visit(*this);
        _os << ")";
    }
}

void QueryPrinter::accept(const QuasiLivenessCondition *element) {
    const Condition_ptr &cond = element->get_compiled();
    if (cond) {
        cond->visit(*this);
    } else {
        _os << "liveness";
    }
}

void QueryPrinter::accept(const StableMarkingCondition *element) {
    const Condition_ptr &cond = element->get_compiled();
    if (cond) {
        cond->visit(*this);
    } else {
        _os << "stable-marking";
    }
}

void QueryPrinter::accept(const BooleanCondition *element) {
    _os << (element->_value ? "true" : "false");
}

void QueryPrinter::accept(const UnfoldedIdentifierExpr *element) { _os << element->name(); }

void QueryPrinter::accept(const LiteralExpr *element) { _os << element->value(); }

void QueryPrinter::accept(const CommutativeExpr *element, const std::string &op) {
    _os << "(" << element->constant();
    for (const auto &id : element->places()) {
        _os << " " << op << " " << id.second;
    }
    for (const auto &expr : element->expressions()) {
        _os << " " << op << " ";
        expr->visit(*this);
    }
    _os << ")";
}

void QueryPrinter::accept(const NaryExpr *element, const std::string &op) {
    _os << "(";
    (*element)[0]->visit(*this);
    for (size_t i = 1; i < element->operands(); ++i) {
        _os << " " << op << " ";
        (*element)[i]->visit(*this);
    }
    _os << ")";
}

void QueryPrinter::accept(const PlusExpr *element) {
    accept((const CommutativeExpr *)element, "+");
}

void QueryPrinter::accept(const MultiplyExpr *element) {
    accept((const CommutativeExpr *)element, "*");
}

void QueryPrinter::accept(const MinusExpr *element) {
    _os << "-";
    (*element)[0]->visit(*this);
}

void QueryPrinter::accept(const SubtractExpr *element) { accept((const NaryExpr *)element, "-"); }

void QueryPrinter::accept(const IdentifierExpr *element) { _os << element->name(); }
} // namespace PetriEngine::PQL