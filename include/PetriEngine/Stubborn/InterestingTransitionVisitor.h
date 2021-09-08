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

#ifndef VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
#define VERIFYPN_INTERESTINGTRANSITIONVISITOR_H

#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/Stubborn/StubbornSet.h"

namespace PetriEngine {
class InterestingTransitionVisitor : public PQL::Visitor {
  public:
    InterestingTransitionVisitor(PetriEngine::StubbornSet &stubbornSet, bool closure)
        : _stubborn(stubbornSet), _closure(closure), _incr(stubbornSet, closure),
          _decr(stubbornSet, closure) {
        _incr._decr = &_decr;
        _decr._incr = &_incr;
    }

    void negate() { _negated = !_negated; }

    [[nodiscard]] auto get_negated() const -> bool { return _negated; }

  protected:
    PetriEngine::StubbornSet &_stubborn;

    bool _closure;

    void accept(const PQL::NotCondition *element) override;

    void accept(const PQL::AndCondition *element) override;

    void accept(const PQL::OrCondition *element) override;

    void accept(const PQL::LessThanCondition *element) override;

    void accept(const PQL::LessThanOrEqualCondition *element) override;

    void accept(const PQL::EqualCondition *element) override;

    void accept(const PQL::NotEqualCondition *element) override;

    void accept(const PQL::DeadlockCondition *element) override;

    void accept(const PQL::CompareConjunction *element) override;

    void accept(const PQL::UnfoldedUpperBoundsCondition *element) override;

    void accept(const PQL::UnfoldedIdentifierExpr *element) override {
        assert(false);
        throw base_error_t("No accept for UnfoldedIdentifierExpr");
    };

    void accept(const PQL::LiteralExpr *element) override {
        assert(false);
        throw base_error_t("No accept for LiteralExpr");
    };

    void accept(const PQL::PlusExpr *element) override {
        assert(false);
        throw base_error_t("No accept for PlusExpr");
    };

    void accept(const PQL::MultiplyExpr *element) override {
        assert(false);
        throw base_error_t("No accept for MultiplyExpr");
    };

    void accept(const PQL::MinusExpr *element) override {
        assert(false);
        throw base_error_t("No accept for MinusExpr");
    };

    void accept(const PQL::SubtractExpr *element) override {
        assert(false);
        throw base_error_t("No accept for SubtractExpr");
    };

    void accept(const PQL::SimpleQuantifierCondition *element);

    void accept(const PQL::EFCondition *condition) override;

    void accept(const PQL::EGCondition *condition) override;

    void accept(const PQL::AGCondition *condition) override;

    void accept(const PQL::AFCondition *condition) override;

    void accept(const PQL::EXCondition *condition) override;

    void accept(const PQL::AXCondition *condition) override;

    void accept(const PQL::ACondition *condition) override;

    void accept(const PQL::ECondition *condition) override;

    void accept(const PQL::XCondition *condition) override;

    void accept(const PQL::UntilCondition *element) override;

    void accept(const PQL::GCondition *condition) override;

    void accept(const PQL::FCondition *condition) override;

    void accept(const PQL::EUCondition *condition) override;

    void accept(const PQL::AUCondition *condition) override;

    void accept(const PQL::BooleanCondition *element) override;

    bool _negated = false;

  private:
    /*
     * Mutually recursive visitors for incrementing/decrementing of places.
     */

    class DecrVisitor;

    class IncrVisitor : public PQL::ExpressionVisitor {
      public:
        IncrVisitor(StubbornSet &stubbornSet, bool closure)
            : _stubborn(stubbornSet), _closure(closure) {}

        DecrVisitor *_decr;

      private:
        StubbornSet &_stubborn;
        bool _closure;

        void accept(const PQL::IdentifierExpr *element) override {
            element->compiled()->visit(*this);
        }

        void accept(const PQL::UnfoldedIdentifierExpr *element) override;

        void accept(const PQL::LiteralExpr *element) override;

        void accept(const PQL::PlusExpr *element) override;

        void accept(const PQL::MultiplyExpr *element) override;

        void accept(const PQL::MinusExpr *element) override;

        void accept(const PQL::SubtractExpr *element) override;
    };

    class DecrVisitor : public PQL::ExpressionVisitor {
      public:
        DecrVisitor(StubbornSet &stubbornSet, bool closure)
            : _stubborn(stubbornSet), _closure(closure) {}

        IncrVisitor *_incr;

      private:
        StubbornSet &_stubborn;
        bool _closure;

        void accept(const PQL::IdentifierExpr *element) override {
            element->compiled()->visit(*this);
        }

        void accept(const PQL::UnfoldedIdentifierExpr *element) override;

        void accept(const PQL::LiteralExpr *element) override;

        void accept(const PQL::PlusExpr *element) override;

        void accept(const PQL::MultiplyExpr *element) override;

        void accept(const PQL::MinusExpr *element) override;

        void accept(const PQL::SubtractExpr *element) override;
    };

    IncrVisitor _incr;
    DecrVisitor _decr;
};

class InterestingLTLTransitionVisitor : public InterestingTransitionVisitor {
  public:
    explicit InterestingLTLTransitionVisitor(StubbornSet &stubbornSet, bool closure)
        : InterestingTransitionVisitor(stubbornSet, closure) {}

  protected:
    void accept(const PQL::LessThanCondition *element) override;

    void accept(const PQL::LessThanOrEqualCondition *element) override;

    void accept(const PQL::EqualCondition *element) override;

    void accept(const PQL::NotEqualCondition *element) override;

    void accept(const PQL::CompareConjunction *element) override;

    template <typename Condition> void negate_if_satisfied(const Condition *element);
};

class AutomatonInterestingTransitionVisitor : public InterestingTransitionVisitor {
  public:
    explicit AutomatonInterestingTransitionVisitor(StubbornSet &stubbornSet, bool closure)
        : InterestingTransitionVisitor(stubbornSet, closure) {}

  protected:
    void accept(const PQL::CompareConjunction *element) override;
};
} // namespace PetriEngine

#endif // VERIFYPN_INTERESTINGTRANSITIONVISITOR_H
