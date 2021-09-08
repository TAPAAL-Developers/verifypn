/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   PlaceUseVisitor.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 23, 2020, 8:44 PM
 */

#ifndef PLACEUSEVISITOR_H
#define PLACEUSEVISITOR_H

#include "PetriEngine/PQL/Visitor.h"

#include <vector>

namespace PetriEngine::PQL {
class PlaceUseVisitor : public Visitor {
  private:
    std::vector<bool> _in_use;

  public:
    PlaceUseVisitor(size_t places);

    [[nodiscard]] auto in_use() const -> const std::vector<bool> { return _in_use; }

  protected:
    void accept(const NotCondition *element) override;
    void accept(const AndCondition *element) override;
    void accept(const OrCondition *element) override;
    void accept(const LessThanCondition *element) override;
    void accept(const LessThanOrEqualCondition *element) override;
    void accept(const EqualCondition *element) override;
    void accept(const NotEqualCondition *element) override;
    void accept(const LiteralExpr *element) override;
    void accept(const UnfoldedIdentifierExpr *element) override;
    void accept(const PlusExpr *element) override;
    void accept(const MultiplyExpr *element) override;
    void accept(const MinusExpr *element) override;
    void accept(const SubtractExpr *element) override;
    void accept(const DeadlockCondition *element) override;
    void accept(const CompareConjunction *element) override;
    void accept(const UnfoldedUpperBoundsCondition *element) override;

    void accept(const EFCondition *el) override;
    void accept(const EGCondition *el) override;
    void accept(const AGCondition *el) override;
    void accept(const AFCondition *el) override;
    void accept(const EXCondition *el) override;
    void accept(const AXCondition *el) override;
    void accept(const EUCondition *el) override;
    void accept(const AUCondition *el) override;

  private:
    void visit_commutative_expr(const CommutativeExpr *element);
};
} // namespace PetriEngine::PQL

#endif /* PLACEUSEVISITOR_H */
