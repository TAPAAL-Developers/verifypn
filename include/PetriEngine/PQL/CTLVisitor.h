#ifndef VERIFYPN_CTLVISITOR_H
#define VERIFYPN_CTLVISITOR_H

#include "Visitor.h"

namespace PetriEngine::PQL {

enum ctl_syntax_type_e { BOOLEAN, PATH, ERROR = -1 };

class IsCTLVisitor : public Visitor {
  public:
    bool _is_CTL = true;

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

  private:
    ctl_syntax_type_e _cur_type;

    void accept(const LogicalCondition *element);

    void accept(const CompareCondition *element);
};

class AsCTL : public Visitor {
  public:
    Condition_ptr _ctl_query = nullptr;
    Expr_ptr _expression = nullptr;

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

  private:
    auto compare_condition(const CompareCondition *element) -> std::pair<Expr_ptr, Expr_ptr>;

    template <typename T> void accept_nary(const T *element);

    template <typename T> auto copy_narry_expr(const T *el) -> Expr_ptr;

    template <typename T> auto copy_compare_condition(const T *element) -> std::shared_ptr<T>;
};
} // namespace PetriEngine::PQL

#endif // VERIFYPN_CTLVISITOR_H
