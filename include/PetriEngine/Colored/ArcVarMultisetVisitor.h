/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */

#ifndef VERIFYPN_ARCVARMULTISETVISITOR_H
#define VERIFYPN_ARCVARMULTISETVISITOR_H

#include "Expressions.h"
#include "ColorExpressionVisitor.h"
#include "VarMultiset.h"

#include <utils/errors.h>
#include <optional>

namespace PetriEngine::Colored {

    std::optional<VarMultiset> extractVarMultiset(const ArcExpression &e);

    class ArcVarMultisetVisitor : public ColorExpressionVisitor {
    public:
        ArcVarMultisetVisitor() : _msRes(), _tupRes(), _varRes() {};

        void accept(const DotConstantExpression *) override {
            _ok = false;
        };

        void accept(const VariableExpression *) override;

        void accept(const UserOperatorExpression *) override {
            _ok = false;
        };

        void accept(const SuccessorExpression *) override {
            _ok = false;
        };

        void accept(const PredecessorExpression *) override {
            _ok = false;
        };

        void accept(const TupleExpression *) override;

        void accept(const LessThanExpression *) override {
            _ok = false;
        };

        void accept(const LessThanEqExpression *) override {
            _ok = false;
        };

        void accept(const EqualityExpression *) override {
            _ok = false;
        };

        void accept(const InequalityExpression *) override {
            _ok = false;
        };

        void accept(const AndExpression *) override {
            _ok = false;
        };

        void accept(const OrExpression *) override {
            _ok = false;
        };

        void accept(const AllExpression *) override {
            _ok = false;
        };

        void accept(const NumberOfExpression *) override;

        void accept(const AddExpression *) override;

        void accept(const SubtractExpression *) override {
            _ok = false;
        };

        void accept(const ScalarProductExpression *) override;

        std::optional<VarMultiset> getResult() const;

    private:
        bool _ok = true; // False if the arc expression could not be converted to multiset of variables
        VarMultiset _msRes;
        bool _inTuple = false;
        std::vector<const Variable *> _tupRes;
        const Variable * _varRes;
    };
}

#endif //VERIFYPN_ARCVARMULTISETVISITOR_H
