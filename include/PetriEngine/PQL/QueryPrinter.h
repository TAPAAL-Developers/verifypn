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

#ifndef VERIFYPN_QUERYPRINTER_H
#define VERIFYPN_QUERYPRINTER_H

#include "Visitor.h"

#include <iostream>
#include <string>

namespace PetriEngine::PQL {
class QueryPrinter : public Visitor {
  public:
    QueryPrinter(std::ostream &os = std::cout) : _os(os) {}

  protected:
    void accept(const NotCondition *element) override;

    void accept(const AndCondition *element) override;

    void accept(const OrCondition *element) override;

    void accept(const LessThanCondition *element) override;

    void accept(const LessThanOrEqualCondition *element) override;

    void accept(const EqualCondition *element) override;

    void accept(const NotEqualCondition *element) override;

    void accept(const DeadlockCondition *element) override;

    void accept(const CompareConjunction *element) override;

    void accept(const UnfoldedUpperBoundsCondition *element) override;

    void accept(const EFCondition *condition) override;

    void accept(const EGCondition *condition) override;

    void accept(const AGCondition *condition) override;

    void accept(const AFCondition *condition) override;

    void accept(const EXCondition *condition) override;

    void accept(const AXCondition *condition) override;

    void accept(const EUCondition *condition) override;

    void accept(const AUCondition *condition) override;

    void accept(const ACondition *condition) override;

    void accept(const ECondition *condition) override;

    void accept(const GCondition *condition) override;

    void accept(const FCondition *condition) override;

    void accept(const XCondition *condition) override;

    void accept(const UntilCondition *condition) override;

    void accept(const UnfoldedFireableCondition *element) override;

    void accept(const FireableCondition *element) override;

    void accept(const UpperBoundsCondition *element) override;

    void accept(const LivenessCondition *element) override;

    void accept(const KSafeCondition *element) override;

    void accept(const QuasiLivenessCondition *element) override;

    void accept(const StableMarkingCondition *element) override;

    void accept(const BooleanCondition *element) override;

    void accept(const UnfoldedIdentifierExpr *element) override;

    void accept(const LiteralExpr *element) override;

    void accept(const PlusExpr *element) override;

    void accept(const MultiplyExpr *element) override;

    void accept(const MinusExpr *element) override;

    void accept(const SubtractExpr *element) override;

    void accept(const IdentifierExpr *element) override;

  protected:
    std::ostream &_os;

    void accept(const LogicalCondition *element, const std::string &op);

    void accept(const CompareCondition *element, const std::string &op);

    void accept(const CommutativeExpr *element, const std::string &op);

    void accept(const NaryExpr *element, const std::string &op);
};
} // namespace PetriEngine::PQL

#endif // VERIFYPN_QUERYPRINTER_H
