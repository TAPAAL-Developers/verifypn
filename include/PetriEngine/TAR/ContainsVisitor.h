/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   ContainsVisitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 21, 2020, 10:44 PM
 */

#ifndef CONTAINSVISITOR_H
#define CONTAINSVISITOR_H

#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/TAR/range.h"

#include <type_traits>

namespace PetriEngine {
using namespace Reachability;

using namespace PQL;
template <typename T> class ContainsVisitor : public Visitor {
  public:
    ContainsVisitor() = default;
    [[nodiscard]] auto does_contain() const -> bool { return _value; }

  private:
    bool _value = false;

  protected:
    template <typename K> auto found_type(K element) -> bool {
        if (std::is_same<const T *, K>::value)
            _value = true;
        return _value;
    }

    void accept(const NotCondition *element) override {
        if (found_type(element))
            return;
        (*element)[0]->visit(*this);
    }
    void accept(const PetriEngine::PQL::AndCondition *element) override {
        if (found_type(element))
            return;
        for (auto &e : *element) {
            e->visit(*this);
            if (_value)
                return;
        }
    }

    void accept(const OrCondition *element) override {
        if (found_type(element))
            return;
        for (auto &e : *element) {
            e->visit(*this);
            if (_value)
                return;
        }
    }

    template <typename K> void handle_compare(const CompareCondition *element) {
        if (found_type(element))
            return;
        (*element)[0]->visit(*this);
        if (_value)
            return;
        (*element)[1]->visit(*this);
    }

    void accept(const LessThanCondition *element) override {
        handle_compare<decltype(element)>(element);
    }

    void accept(const LessThanOrEqualCondition *element) override {
        handle_compare<decltype(element)>(element);
    }

    void accept(const NotEqualCondition *element) override {
        handle_compare<decltype(element)>(element);
    }

    void accept(const EqualCondition *element) override {
        handle_compare<decltype(element)>(element);
    }

    void accept(const IdentifierExpr *element) override {
        if (found_type(element))
            return;
    }

    void accept(const LiteralExpr *element) override {
        if (found_type(element))
            return;
    }

    void accept(const UnfoldedIdentifierExpr *element) override {
        if (found_type(element))
            return;
    }

    template <typename K> void handle_nary_expr(K element) {
        if (found_type(element))
            return;
        for (auto &e : element->expressions()) {
            e->visit(*this);
            if (_value)
                return;
        }
    }

    void accept(const PlusExpr *element) override { handle_nary_expr<decltype(element)>(element); }

    void accept(const MultiplyExpr *element) override {
        handle_nary_expr<decltype(element)>(element);
    }

    void accept(const MinusExpr *element) override {
        if (found_type(element))
            return;
        (*element)[0]->visit(*this);
    }

    void accept(const SubtractExpr *element) override {
        handle_nary_expr<decltype(element)>(element);
    }

    void accept(const DeadlockCondition *element) override {
        if (found_type(element))
            return;
    }

    void accept(const CompareConjunction *element) override {
        if (found_type(element))
            return;
    }

    void accept(const UnfoldedUpperBoundsCondition *element) override {
        if (found_type(element))
            return;
    }

    template <typename K> void handle_simple_quantifier_condition(K element) {
        if (found_type(element))
            return;
        (*element)[0]->visit(*this);
    }

    void accept(const EFCondition *el) override { handle_simple_quantifier_condition(el); }
    void accept(const EGCondition *el) override { handle_simple_quantifier_condition(el); }
    void accept(const AGCondition *el) override { handle_simple_quantifier_condition(el); }
    void accept(const AFCondition *el) override { handle_simple_quantifier_condition(el); }
    void accept(const EXCondition *el) override { handle_simple_quantifier_condition(el); }
    void accept(const AXCondition *el) override { handle_simple_quantifier_condition(el); }

    void accept(const EUCondition *el) override {
        if (found_type(el))
            return;
        (*el)[0]->visit(*this);
        if (_value)
            return;
        (*el)[1]->visit(*this);
    }

    void accept(const AUCondition *el) override {
        if (found_type(el))
            return;
        (*el)[0]->visit(*this);
        if (_value)
            return;
        (*el)[1]->visit(*this);
    }

    // shallow elements, neither of these should exist in a compiled expression
    void accept(const UnfoldedFireableCondition *element) override { found_type(element); };
    void accept(const FireableCondition *element) override { found_type(element); };
    void accept(const UpperBoundsCondition *element) override { found_type(element); };
    void accept(const LivenessCondition *element) override { found_type(element); };
    void accept(const KSafeCondition *element) override { found_type(element); };
    void accept(const QuasiLivenessCondition *element) override { found_type(element); };
    void accept(const StableMarkingCondition *element) override { found_type(element); };
    void accept(const BooleanCondition *element) override { found_type(element); };
};
} // namespace PetriEngine

#endif /* CONTAINSVISITOR_H */
