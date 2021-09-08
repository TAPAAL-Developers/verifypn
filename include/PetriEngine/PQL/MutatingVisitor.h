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

#ifndef VERIFYPN_MUTATINGVISITOR_H
#define VERIFYPN_MUTATINGVISITOR_H

#include "PetriEngine/PQL/Expressions.h"
#include "errorcodes.h"

namespace PetriEngine::PQL {
class MutatingVisitor {
  public:
    MutatingVisitor() = default;

    template <typename T> void accept(T &&element) { accept(element); }

  protected:
    virtual void accept(NotCondition *element) = 0;

    virtual void accept(AndCondition *element) = 0;

    virtual void accept(OrCondition *element) = 0;

    virtual void accept(LessThanCondition *element) = 0;

    virtual void accept(LessThanOrEqualCondition *element) = 0;

    virtual void accept(EqualCondition *element) = 0;

    virtual void accept(NotEqualCondition *element) = 0;

    virtual void accept(DeadlockCondition *element) = 0;

    virtual void accept(CompareConjunction *element) = 0;

    virtual void accept(UnfoldedUpperBoundsCondition *element) = 0;

    // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default
    // implementation. default behaviour is error
    virtual void accept(EFCondition *) {
        assert(false);
        throw base_error_t("No accept for EFCondition");
    };

    virtual void accept(EGCondition *) {
        assert(false);
        throw base_error_t("No accept for EGCondition");
    };

    virtual void accept(AGCondition *) {
        assert(false);
        throw base_error_t("No accept for AGCondition");
    };

    virtual void accept(AFCondition *) {
        assert(false);
        throw base_error_t("No accept for AFCondition");
    };

    virtual void accept(EXCondition *) {
        assert(false);
        throw base_error_t("No accept for EXCondition");
    };

    virtual void accept(AXCondition *) {
        assert(false);
        throw base_error_t("No accept for AXCondition");
    };

    virtual void accept(EUCondition *) {
        assert(false);
        throw base_error_t("No accept for EUCondition");
    };

    virtual void accept(AUCondition *) {
        assert(false);
        throw base_error_t("No accept for AUCondition");
    };

    virtual void accept(ACondition *) {
        assert(false);
        throw base_error_t("No accept for ACondition");
    };

    virtual void accept(ECondition *) {
        assert(false);
        throw base_error_t("No accept for ECondition");
    };

    virtual void accept(GCondition *) {
        assert(false);
        throw base_error_t("No accept for GCondition");
    };

    virtual void accept(FCondition *) {
        assert(false);
        throw base_error_t("No accept for FCondition");
    };

    virtual void accept(XCondition *) {
        assert(false);
        throw base_error_t("No accept for XCondition");
    };

    virtual void accept(UntilCondition *) {
        assert(false);
        throw base_error_t("No accept for UntilCondition");
    };

    // shallow elements, neither of these should exist in a compiled expression
    virtual void accept(UnfoldedFireableCondition *element) {
        assert(false);
        throw base_error_t("No accept for UnfoldedFireableCondition");
    };

    virtual void accept(FireableCondition *element) {
        assert(false);
        throw base_error_t("No accept for FireableCondition");
    };

    virtual void accept(UpperBoundsCondition *element) {
        assert(false);
        throw base_error_t("No accept for UpperBoundsCondition");
    };

    virtual void accept(LivenessCondition *element) {
        assert(false);
        throw base_error_t("No accept for LivenessCondition");
    };

    virtual void accept(KSafeCondition *element) {
        assert(false);
        throw base_error_t("No accept for KSafeCondition");
    };

    virtual void accept(QuasiLivenessCondition *element) {
        assert(false);
        throw base_error_t("No accept for QuasiLivenessCondition");
    };

    virtual void accept(StableMarkingCondition *element) {
        assert(false);
        throw base_error_t("No accept for StableMarkingCondition");
    };

    virtual void accept(BooleanCondition *element) {
        assert(false);
        throw base_error_t("No accept for BooleanCondition");
    };

    // Expression
    virtual void accept(UnfoldedIdentifierExpr *element) {
        assert(false);
        throw base_error_t("No accept for UnfoldedIdentifierExpr");
    };

    virtual void accept(LiteralExpr *element) {
        assert(false);
        throw base_error_t("No accept for LiteralExpr");
    };

    virtual void accept(PlusExpr *element) {
        assert(false);
        throw base_error_t("No accept for PlusExpr");
    };

    virtual void accept(MultiplyExpr *element) {
        assert(false);
        throw base_error_t("No accept for MultiplyExpr");
    };

    virtual void accept(MinusExpr *element) {
        assert(false);
        throw base_error_t("No accept for MinusExpr");
    };

    virtual void accept(SubtractExpr *element) {
        assert(false);
        throw base_error_t("No accept for SubtractExpr");
    };

    // shallow expression, default to error
    virtual void accept(IdentifierExpr *element) {
        assert(false);
        throw base_error_t("No accept for IdentifierExpr");
    };
};
} // namespace PetriEngine::PQL
#endif // VERIFYPN_MUTATINGVISITOR_H
