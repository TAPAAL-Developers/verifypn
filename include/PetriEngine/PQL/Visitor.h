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

namespace PetriEngine {
    namespace PQL {
        class Visitor {
        public:
            Visitor() {}

            template<typename T>
            void accept(T &&element) {
                _accept(element);
            }

            //virtual ~Visitor() = default;

        protected:
            virtual void _accept(const NotCondition* element) = 0;
            virtual void _accept(const AndCondition* element) = 0;
            virtual void _accept(const OrCondition* element) = 0;
            virtual void _accept(const LessThanCondition* element) = 0;
            virtual void _accept(const LessThanOrEqualCondition* element) = 0;
            virtual void _accept(const EqualCondition* element) = 0;
            virtual void _accept(const NotEqualCondition* element) = 0;

            virtual void _accept(const DeadlockCondition* element) = 0;
            virtual void _accept(const CompareConjunction* element) = 0;
            virtual void _accept(const UnfoldedUpperBoundsCondition* element) = 0;

            // Quantifiers, most uses of the visitor will not use the quantifiers - so we give a default implementation.
            // default behaviour is error
            virtual void _accept(const EFCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for EFCondition");

            };

            virtual void _accept(const EGCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for EGCondition");

            };

            virtual void _accept(const AGCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for AGCondition");

            };

            virtual void _accept(const AFCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for AFCondition");

            };

            virtual void _accept(const EXCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for EXCondition");

            };

            virtual void _accept(const AXCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for AXCondition");

            };

            virtual void _accept(const EUCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for EUCondition");

            };

            virtual void _accept(const AUCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for AUCondition");

            };

            virtual void _accept(const ACondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for ACondition");

            };

            virtual void _accept(const ECondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for ECondition");

            };

            virtual void _accept(const GCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for GCondition");

            };

            virtual void _accept(const FCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for FCondition");

            };

            virtual void _accept(const XCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for XCondition");

            };

            virtual void _accept(const UntilCondition *) {
                assert(false);
                throw base_error(ErrorCode, "No accept for UntilCondition");

            };

            // shallow elements, neither of these should exist in a compiled expression
            virtual void _accept(const UnfoldedFireableCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for UnfoldedFireableCondition");

            };

            virtual void _accept(const FireableCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for FireableCondition");

            };

            virtual void _accept(const UpperBoundsCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for UpperBoundsCondition");

            };

            virtual void _accept(const LivenessCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for LivenessCondition");

            };

            virtual void _accept(const KSafeCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for KSafeCondition");

            };

            virtual void _accept(const QuasiLivenessCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for QuasiLivenessCondition");

            };

            virtual void _accept(const StableMarkingCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for StableMarkingCondition");

            };

            virtual void _accept(const BooleanCondition *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for BooleanCondition");

            };

            // Expression
            virtual void _accept(const UnfoldedIdentifierExpr *element) = 0;

            virtual void _accept(const LiteralExpr *element) = 0;

            virtual void _accept(const PlusExpr *element) = 0;

            virtual void _accept(const MultiplyExpr *element) = 0;

            virtual void _accept(const MinusExpr *element) = 0;

            virtual void _accept(const SubtractExpr *element) = 0;

            // shallow expression, default to error
            virtual void _accept(const IdentifierExpr *element) {
                assert(false);
                throw base_error(ErrorCode, "No accept for IdentifierExpr");

            };
        };

        class ExpressionVisitor : public Visitor {
        public:

        private:
            void _accept(const NotCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for NotCondition");

            };

            void _accept(const AndCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for AndCondition");

            };

            void _accept(const OrCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for OrCondition");

            };

            void _accept(const LessThanCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for LessThanCondition");

            };

            void _accept(const LessThanOrEqualCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for LessThanOrEqualCondition");

            };

            void _accept(const EqualCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for EqualCondition");

            };

            void _accept(const NotEqualCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for NotEqualCondition");

            };

            void _accept(const DeadlockCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for DeadlockCondition");

            };

            void _accept(const CompareConjunction *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for CompareConjunction");

            };

            void _accept(const UnfoldedUpperBoundsCondition *element) override {
                assert(false);
                throw base_error(ErrorCode, "No accept for UnfoldedUpperBoundsCondition");

            };
        };

        class BaseVisitor : public Visitor {
        protected:
            void _accept(const NotCondition *element) override
            {
                (*element)[0]->visit(*this);
            }

            void _accept(const AndCondition *element) override
            {
                for (const auto &cond : *element) {
                    cond->visit(*this);
                }
            }

            void _accept(const OrCondition *element) override
            {
                for (const auto &cond : *element) {
                    cond->visit(*this);
                }
            }

            void _accept(const LessThanCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const LessThanOrEqualCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const EqualCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const NotEqualCondition *element) override
            {
                element->getExpr1()->visit(*this);
                element->getExpr2()->visit(*this);
            }

            void _accept(const DeadlockCondition *element) override
            {
                // no-op
            }

            void _accept(const CompareConjunction *element) override
            {
                // no-op, complicated
            }

            void _accept(const UnfoldedUpperBoundsCondition *element) override
            {
                // no-op
            }

            void _accept(const EFCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EGCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AGCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AFCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EXCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const AXCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const EUCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const AUCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const ACondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const ECondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const GCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const FCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const XCondition *condition) override
            {
                (*condition)[0]->visit(*this);
            }

            void _accept(const UntilCondition *condition) override
            {
                (*condition)[0]->visit(*this);
                (*condition)[1]->visit(*this);
            }

            void _accept(const UnfoldedFireableCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const FireableCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const UpperBoundsCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const LivenessCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const KSafeCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const QuasiLivenessCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const StableMarkingCondition *element) override
            {
                if (const auto &compiled = element->get_compiled())
                    compiled->visit(*this);
            }

            void _accept(const BooleanCondition *element) override
            {
                // no-op
            }

            void _accept(const UnfoldedIdentifierExpr *element) override
            {
                // no-op
            }

            void _accept(const LiteralExpr *element) override
            {
                // no-op
            }

            void _accept(const PlusExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const MultiplyExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const MinusExpr *element) override
            {
                (*element)[0]->visit(*this);
            }

            void _accept(const SubtractExpr *element) override
            {
                for (const auto &expr : element->expressions()) {
                    expr->visit(*this);
                }
            }

            void _accept(const IdentifierExpr *element) override
            {
                // no-op
            }
        };
    }
}

#endif /* VISITOR_H */

