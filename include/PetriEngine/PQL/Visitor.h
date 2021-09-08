/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   Visitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 11, 2020, 1:07 PM
 */

#ifndef VISITOR_H
#define VISITOR_H

#include "PetriEngine/PQL/Expressions.h"
#include "errorcodes.h"
#include <type_traits>

namespace PetriEngine::PQL {
class Visitor {
  public:
    Visitor() = default;

    template <typename T> void accept(T &&element) { accept(element); }

    // virtual ~Visitor() = default;

  protected:
    virtual void accept(const NotCondition *element) = 0;
    virtual void accept(const AndCondition *element) = 0;
    virtual void accept(const OrCondition *element) = 0;
    virtual void accept(const LessThanCondition *element) = 0;
    virtual void accept(const LessThanOrEqualCondition *element) = 0;
    virtual void accept(const EqualCondition *element) = 0;
    virtual void accept(const NotEqualCondition *element) = 0;

    virtual void accept(const DeadlockCondition *element) = 0;
    virtual void accept(const CompareConjunction *element) = 0;
    virtual void accept(const UnfoldedUpperBoundsCondition *element) = 0;

    // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default
    // implementation. default behaviour is error
    virtual void accept(const EFCondition *) {
        assert(false);
        throw base_error_t("No accept for EFCondition");
    };

    virtual void accept(const EGCondition *) {
        assert(false);
        throw base_error_t("No accept for EGCondition");
    };

    virtual void accept(const AGCondition *) {
        assert(false);
        throw base_error_t("No accept for AGCondition");
    };

    virtual void accept(const AFCondition *) {
        assert(false);
        throw base_error_t("No accept for AFCondition");
    };

    virtual void accept(const EXCondition *) {
        assert(false);
        throw base_error_t("No accept for EXCondition");
    };

    virtual void accept(const AXCondition *) {
        assert(false);
        throw base_error_t("No accept for AXCondition");
    };

    virtual void accept(const EUCondition *) {
        assert(false);
        throw base_error_t("No accept for EUCondition");
    };

    virtual void accept(const AUCondition *) {
        assert(false);
        throw base_error_t("No accept for AUCondition");
    };

    virtual void accept(const ACondition *) {
        assert(false);
        throw base_error_t("No accept for ACondition");
    };

    virtual void accept(const ECondition *) {
        assert(false);
        throw base_error_t("No accept for ECondition");
    };

    virtual void accept(const GCondition *) {
        assert(false);
        throw base_error_t("No accept for GCondition");
    };

    virtual void accept(const FCondition *) {
        assert(false);
        throw base_error_t("No accept for FCondition");
    };

    virtual void accept(const XCondition *) {
        assert(false);
        throw base_error_t("No accept for XCondition");
    };

    virtual void accept(const UntilCondition *) {
        assert(false);
        throw base_error_t("No accept for UntilCondition");
    };

    // shallow elements, neither of these should exist in a compiled expression
    virtual void accept(const UnfoldedFireableCondition *element) {
        assert(false);
        throw base_error_t("No accept for UnfoldedFireableCondition");
    };

    virtual void accept(const FireableCondition *element) {
        assert(false);
        throw base_error_t("No accept for FireableCondition");
    };

    virtual void accept(const UpperBoundsCondition *element) {
        assert(false);
        throw base_error_t("No accept for UpperBoundsCondition");
    };

    virtual void accept(const LivenessCondition *element) {
        assert(false);
        throw base_error_t("No accept for LivenessCondition");
    };

    virtual void accept(const KSafeCondition *element) {
        assert(false);
        throw base_error_t("No accept for KSafeCondition");
    };

    virtual void accept(const QuasiLivenessCondition *element) {
        assert(false);
        throw base_error_t("No accept for QuasiLivenessCondition");
    };

    virtual void accept(const StableMarkingCondition *element) {
        assert(false);
        throw base_error_t("No accept for StableMarkingCondition");
    };

    virtual void accept(const BooleanCondition *element) {
        assert(false);
        throw base_error_t("No accept for BooleanCondition");
    };

    // Expression
    virtual void accept(const UnfoldedIdentifierExpr *element) = 0;

    virtual void accept(const LiteralExpr *element) = 0;

    virtual void accept(const PlusExpr *element) = 0;

    virtual void accept(const MultiplyExpr *element) = 0;

    virtual void accept(const MinusExpr *element) = 0;

    virtual void accept(const SubtractExpr *element) = 0;

    // shallow expression, default to error
    virtual void accept(const IdentifierExpr *element) {
        assert(false);
        throw base_error_t("No accept for IdentifierExpr");
    };
};

class ExpressionVisitor : public Visitor {
  public:
  private:
    void accept(const NotCondition *element) override {
        assert(false);
        throw base_error_t("No accept for NotCondition");
    };

    void accept(const AndCondition *element) override {
        assert(false);
        throw base_error_t("No accept for AndCondition");
    };

    void accept(const OrCondition *element) override {
        assert(false);
        throw base_error_t("No accept for OrCondition");
    };

    void accept(const LessThanCondition *element) override {
        assert(false);
        throw base_error_t("No accept for LessThanCondition");
    };

    void accept(const LessThanOrEqualCondition *element) override {
        assert(false);
        throw base_error_t("No accept for LessThanOrEqualCondition");
    };

    void accept(const EqualCondition *element) override {
        assert(false);
        throw base_error_t("No accept for EqualCondition");
    };

    void accept(const NotEqualCondition *element) override {
        assert(false);
        throw base_error_t("No accept for NotEqualCondition");
    };

    void accept(const DeadlockCondition *element) override {
        assert(false);
        throw base_error_t("No accept for DeadlockCondition");
    };

    void accept(const CompareConjunction *element) override {
        assert(false);
        throw base_error_t("No accept for CompareConjunction");
    };

    void accept(const UnfoldedUpperBoundsCondition *element) override {
        assert(false);
        throw base_error_t("No accept for UnfoldedUpperBoundsCondition");
    };
};

class BaseVisitor : public Visitor {
  protected:
    void accept(const NotCondition *element) override { (*element)[0]->visit(*this); }

    void accept(const AndCondition *element) override {
        for (const auto &cond : *element) {
            cond->visit(*this);
        }
    }

    void accept(const OrCondition *element) override {
        for (const auto &cond : *element) {
            cond->visit(*this);
        }
    }

    void accept(const LessThanCondition *element) override {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void accept(const LessThanOrEqualCondition *element) override {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void accept(const EqualCondition *element) override {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void accept(const NotEqualCondition *element) override {
        (*element)[0]->visit(*this);
        (*element)[1]->visit(*this);
    }

    void accept(const DeadlockCondition *element) override {
        // no-op
    }

    void accept(const CompareConjunction *element) override {
        // no-op, complicated
    }

    void accept(const UnfoldedUpperBoundsCondition *element) override {
        // no-op
    }

    void accept(const EFCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const EGCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const AGCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const AFCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const EXCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const AXCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const EUCondition *condition) override {
        (*condition)[0]->visit(*this);
        (*condition)[1]->visit(*this);
    }

    void accept(const AUCondition *condition) override {
        (*condition)[0]->visit(*this);
        (*condition)[1]->visit(*this);
    }

    void accept(const ACondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const ECondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const GCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const FCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const XCondition *condition) override { (*condition)[0]->visit(*this); }

    void accept(const UntilCondition *condition) override {
        (*condition)[0]->visit(*this);
        (*condition)[1]->visit(*this);
    }

    void accept(const UnfoldedFireableCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const FireableCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const UpperBoundsCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const LivenessCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const KSafeCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const QuasiLivenessCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const StableMarkingCondition *element) override {
        if (const auto &compiled = element->get_compiled())
            compiled->visit(*this);
    }

    void accept(const BooleanCondition *element) override {
        // no-op
    }

    void accept(const UnfoldedIdentifierExpr *element) override {
        // no-op
    }

    void accept(const LiteralExpr *element) override {
        // no-op
    }

    void accept(const PlusExpr *element) override {
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void accept(const MultiplyExpr *element) override {
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void accept(const MinusExpr *element) override { (*element)[0]->visit(*this); }

    void accept(const SubtractExpr *element) override {
        for (const auto &expr : element->expressions()) {
            expr->visit(*this);
        }
    }

    void accept(const IdentifierExpr *element) override {
        // no-op
    }
};
} // namespace PetriEngine::PQL

#endif /* VISITOR_H */
