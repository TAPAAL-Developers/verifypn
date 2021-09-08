/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   RangeEvalContext.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 11, 2020, 1:36 PM
 */

#ifndef RANGEEVALCONTEXT_H
#define RANGEEVALCONTEXT_H
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Visitor.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/TAR/range.h"

#include <type_traits>

namespace PetriEngine {
using namespace Reachability;

using namespace PQL;

class RangeEvalContext : public Visitor {
  public:
    RangeEvalContext(const prvector_t &vector, const PetriNet &net, const uint64_t *use_count);
    [[nodiscard]] auto satisfied() const -> bool { return _bool_result; }
    [[nodiscard]] auto constraint() const -> const prvector_t & { return _sufficient; }

  private:
    void handle_compare(const Expr_ptr &left, const Expr_ptr &right, bool strict);
    const prvector_t &_ranges;
    const PetriNet &_net;
    const uint64_t *_use_count;
    int64_t _literal;
    bool _bool_result;
    prvector_t _sufficient;
    std::vector<uint32_t> _places;

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
    void accept(const DeadlockCondition *element) override;
    void accept(const CompareConjunction *element) override;
    void accept(const UnfoldedUpperBoundsCondition *element) override;
    void accept(const MultiplyExpr *element) override;
    void accept(const MinusExpr *element) override;
    void accept(const SubtractExpr *element) override;
};
} // namespace PetriEngine

#endif /* RANGEEVALCONTEXT_H */
