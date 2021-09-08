/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   RangeContext.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 5:01 PM
 */

#ifndef RANGECONTEXT_H
#define RANGECONTEXT_H

#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/TAR/range.h"

#include <type_traits>

namespace PetriEngine {
using namespace Reachability;

using namespace PQL;
class RangeContext : public Visitor {
  public:
    RangeContext(prvector_t &vector, MarkVal *base, const PetriNet &net, const uint64_t *uses,
                 MarkVal *marking, const std::vector<bool> &dirty);
    [[nodiscard]] auto is_dirty() const -> bool { return _is_dirty; }

  private:
    void handle_compare(const Expr_ptr &left, const Expr_ptr &right, bool strict);

    prvector_t &_ranges;
    MarkVal *_base;
    const PetriNet &_net;
    const uint64_t *_uses;
    int64_t _limit = 0;
    bool _lt = false;
    MarkVal *_marking;
    bool _is_dirty = false;
    const std::vector<bool> &_dirty;

  protected:
    void accept(const NotCondition *element) override;
    void accept(const PetriEngine::PQL::AndCondition *element) override;
    void accept(const OrCondition *element) override;
    void accept(const LessThanCondition *element) override;
    void accept(const LessThanOrEqualCondition *element) override;
    void accept(const EqualCondition *element) override;
    void accept(const NotEqualCondition *element) override;
    void accept(const LiteralExpr *element) override;
    void accept(const UnfoldedIdentifierExpr *element) override;
    void accept(const PlusExpr *element) override;
    void accept(const MinusExpr *element) override;
    void accept(const SubtractExpr *element) override;
    void accept(const MultiplyExpr *element) override;
    void accept(const DeadlockCondition *element) override;
    void accept(const CompareConjunction *element) override;
    void accept(const UnfoldedUpperBoundsCondition *element) override;
};
} // namespace PetriEngine

#endif /* RANGECONTEXT_H */
