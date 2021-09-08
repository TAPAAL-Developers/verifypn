/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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
#include "PetriEngine/PQL/CTLVisitor.h"

#include <memory>

namespace PetriEngine::PQL {
void IsCTLVisitor::accept(const NotCondition *element) {
    (*element)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const LogicalCondition *element) {
    for (size_t i = 0; i < element->operands(); i++) {
        (*element)[i]->visit(*this);
        if (_cur_type != ctl_syntax_type_e::BOOLEAN) {
            _is_CTL = false;
            break;
        }
    }
}

void IsCTLVisitor::accept(const AndCondition *element) {
    accept((const LogicalCondition *)element);
}

void IsCTLVisitor::accept(const OrCondition *element) {
    accept((const LogicalCondition *)element);
}

void IsCTLVisitor::accept(const CompareCondition *element) {
    // We are an atom. No need to check children as they are the same as CTL*
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const LessThanCondition *element) {
    accept((const CompareCondition *)element);
}

void IsCTLVisitor::accept(const LessThanOrEqualCondition *element) {
    accept((const CompareCondition *)element);
}

void IsCTLVisitor::accept(const EqualCondition *element) {
    accept((const CompareCondition *)element);
}

void IsCTLVisitor::accept(const NotEqualCondition *element) {
    accept((const CompareCondition *)element);
}

void IsCTLVisitor::accept(const DeadlockCondition *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const CompareConjunction *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const UnfoldedUpperBoundsCondition *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const EFCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const EGCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const AGCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const AFCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const EXCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const AXCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const EUCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const AUCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
}

void IsCTLVisitor::accept(const ACondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::PATH)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const ECondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::PATH)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const GCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::PATH;
}

void IsCTLVisitor::accept(const FCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::PATH;
}

void IsCTLVisitor::accept(const XCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::PATH;
}

void IsCTLVisitor::accept(const UntilCondition *condition) {
    (*condition)[0]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
    (*condition)[1]->visit(*this);
    if (_cur_type != ctl_syntax_type_e::BOOLEAN)
        _is_CTL = false;
    _cur_type = ctl_syntax_type_e::PATH;
}

void IsCTLVisitor::accept(const UnfoldedFireableCondition *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const FireableCondition *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const UpperBoundsCondition *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const LivenessCondition *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const KSafeCondition *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const QuasiLivenessCondition *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const StableMarkingCondition *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const BooleanCondition *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const UnfoldedIdentifierExpr *element) {
    _cur_type = ctl_syntax_type_e::BOOLEAN;
}

void IsCTLVisitor::accept(const LiteralExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const PlusExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const MultiplyExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const MinusExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const SubtractExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void IsCTLVisitor::accept(const IdentifierExpr *element) { _cur_type = ctl_syntax_type_e::BOOLEAN; }

void AsCTL::accept(const NotCondition *element) {
    (*element)[0]->visit(*this);
    _ctl_query = std::make_shared<NotCondition>(_ctl_query);
}

template <typename T> void AsCTL::accept_nary(const T *element) {
    std::vector<Condition_ptr> children;
    for (const auto &operand : *element) {
        operand->visit(*this);
        children.push_back(_ctl_query);
    }
    _ctl_query = std::make_shared<T>(children);
}

void AsCTL::accept(const AndCondition *element) { AsCTL::accept_nary(element); }

void AsCTL::accept(const OrCondition *element) { AsCTL::accept_nary(element); }

template <typename T> auto AsCTL::copy_compare_condition(const T *element) -> std::shared_ptr<T> {
    // we copy of sharedptr for now, but this is not safe!
    // copy_narry_expr needs fixing.
    return std::make_shared<T>((*element)[0], (*element)[1]);
}

void AsCTL::accept(const LessThanCondition *element) {
    _ctl_query = copy_compare_condition(element);
}

void AsCTL::accept(const LessThanOrEqualCondition *element) {
    _ctl_query = copy_compare_condition(element);
}

void AsCTL::accept(const EqualCondition *element) { _ctl_query = copy_compare_condition(element); }

void AsCTL::accept(const NotEqualCondition *element) {
    _ctl_query = copy_compare_condition(element);
}

void AsCTL::accept(const DeadlockCondition *element) {
    _ctl_query = std::make_shared<DeadlockCondition>();
}

void AsCTL::accept(const CompareConjunction *element) {
    _ctl_query = std::make_shared<CompareConjunction>(*element);
}

void AsCTL::accept(const UnfoldedUpperBoundsCondition *element) {
    _ctl_query = std::make_shared<UnfoldedUpperBoundsCondition>(*element);
}

void AsCTL::accept(const EFCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<EFCondition>(_ctl_query);
}

void AsCTL::accept(const EGCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<EGCondition>(_ctl_query);
}

void AsCTL::accept(const AGCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<AGCondition>(_ctl_query);
}

void AsCTL::accept(const AFCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<AFCondition>(_ctl_query);
}

void AsCTL::accept(const EXCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<EXCondition>(_ctl_query);
}

void AsCTL::accept(const AXCondition *condition) {
    (*condition)[0]->visit(*this);
    _ctl_query = std::make_shared<AXCondition>(_ctl_query);
}

void AsCTL::accept(const EUCondition *condition) {
    (*condition)[0]->visit(*this);
    auto first = _ctl_query;
    (*condition)[1]->visit(*this);
    _ctl_query = std::make_shared<EUCondition>(first, _ctl_query);
}

void AsCTL::accept(const AUCondition *condition) {
    (*condition)[0]->visit(*this);
    auto first = _ctl_query;
    (*condition)[1]->visit(*this);
    _ctl_query = std::make_shared<AUCondition>(first, _ctl_query);
}

void AsCTL::accept(const ACondition *condition) {
    auto child = dynamic_cast<QuantifierCondition *>((*condition)[0].get());
    switch (child->get_path()) {
    case path_e::G:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<AGCondition>(_ctl_query);
        break;
    case path_e::X:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<AXCondition>(_ctl_query);
        break;
    case path_e::F:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<AFCondition>(_ctl_query);
        break;
    case path_e::U: {
        (*child)[0]->visit(*this);
        auto first = _ctl_query;
        (*child)[1]->visit(*this);
        _ctl_query = std::make_shared<AUCondition>(first, _ctl_query);
        break;
    }
    case path_e::P_ERROR:
        assert(false);
        _ctl_query = nullptr;
        break;
    }
}

void AsCTL::accept(const ECondition *condition) {
    auto child = dynamic_cast<QuantifierCondition *>((*condition)[0].get());
    switch (child->get_path()) {
    case path_e::G:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<EGCondition>(_ctl_query);
        break;
    case path_e::X:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<EXCondition>(_ctl_query);
        break;
    case path_e::F:
        (*child)[0]->visit(*this);
        _ctl_query = std::make_shared<EFCondition>(_ctl_query);
        break;
    case path_e::U: {
        (*child)[0]->visit(*this);
        auto first = _ctl_query;
        (*child)[1]->visit(*this);
        _ctl_query = std::make_shared<EUCondition>(first, _ctl_query);
        break;
    }
    case path_e::P_ERROR:
        assert(false);
        _ctl_query = nullptr;
        break;
    }
}

void AsCTL::accept(const GCondition *condition) {
    std::cerr << "Direct call to path quantifier in AsCTL GCondition" << std::endl;
    assert(false);
    _ctl_query = nullptr;
}

void AsCTL::accept(const FCondition *condition) {
    std::cerr << "Direct call to path quantifier in AsCTL FCondition" << std::endl;
    assert(false);
    _ctl_query = nullptr;
}

void AsCTL::accept(const XCondition *condition) {
    std::cerr << "Direct call to path quantifier in AsCTL XCondition" << std::endl;
    assert(false);
    _ctl_query = nullptr;
}

void AsCTL::accept(const UntilCondition *condition) {
    std::cerr << "Direct call to path quantifier in AsCTL UntilCondition" << std::endl;
    assert(false);
    _ctl_query = nullptr;
}

void AsCTL::accept(const UnfoldedFireableCondition *element) {
    _ctl_query = std::make_shared<UnfoldedFireableCondition>(*element);
}

template <typename T> auto copy_condition(const T *el) -> Condition_ptr {
    return std::make_shared<T>(*el);
}

void AsCTL::accept(const FireableCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const UpperBoundsCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const LivenessCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const KSafeCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const QuasiLivenessCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const StableMarkingCondition *element) { _ctl_query = copy_condition(element); }

void AsCTL::accept(const BooleanCondition *element) {
    _ctl_query =
        element->_value ? BooleanCondition::TRUE_CONSTANT : BooleanCondition::FALSE_CONSTANT;
}

template <typename T> auto AsCTL::copy_narry_expr(const T *el) -> Expr_ptr {
    assert(false);
    // TODO: fix
    return nullptr;
}

void AsCTL::accept(const PlusExpr *element) { _expression = copy_narry_expr(element); }

void AsCTL::accept(const MultiplyExpr *element) { _expression = copy_narry_expr(element); }

void AsCTL::accept(const SubtractExpr *element) { _expression = copy_narry_expr(element); }

void AsCTL::accept(const MinusExpr *element) {
    (*element)[0]->visit(*this);
    _expression = std::make_shared<MinusExpr>(_expression);
}

void AsCTL::accept(const LiteralExpr *element) {
    _expression = std::make_shared<LiteralExpr>(element->value());
}

void AsCTL::accept(const IdentifierExpr *element) {
    _expression = std::make_shared<IdentifierExpr>(*element);
}

void AsCTL::accept(const UnfoldedIdentifierExpr *element) {
    _expression = std::make_shared<UnfoldedIdentifierExpr>(*element);
}

} // namespace PetriEngine::PQL