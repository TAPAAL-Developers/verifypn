/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/MutatingVisitor.h"
#include "PetriEngine/PQL/Visitor.h"
#include "errorcodes.h"

#include "PetriEngine/PQL/QueryPrinter.h"
#include <PetriEngine/Stubborn/StubbornSet.h>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <numeric>
#include <set>

using namespace PetriEngine::Simplification;

namespace PetriEngine::PQL {

auto generate_tabs(std::ostream &out, uint32_t tabs) -> std::ostream & {

    for (uint32_t i = 0; i < tabs; i++) {
        out << "  ";
    }
    return out;
}

/** FOR COMPILING AND CONSTRUCTING LOGICAL OPERATORS **/

template <typename T>
void try_merge(std::vector<Condition_ptr> &_conds, const Condition_ptr &ptr,
               bool aggressive = false) {
    if (auto lor = std::dynamic_pointer_cast<T>(ptr)) {
        for (auto &c : *lor)
            try_merge<T>(_conds, c, aggressive);
    } else if (!aggressive) {
        _conds.emplace_back(ptr);
    } else if (auto comp = std::dynamic_pointer_cast<CompareCondition>(ptr)) {
        if ((std::is_same<T, AndCondition>::value &&
             std::dynamic_pointer_cast<NotEqualCondition>(ptr)) ||
            (std::is_same<T, OrCondition>::value &&
             std::dynamic_pointer_cast<EqualCondition>(ptr))) {
            _conds.emplace_back(ptr);
        } else {
            if (!((dynamic_cast<UnfoldedIdentifierExpr *>((*comp)[0].get()) &&
                   (*comp)[1]->place_free()) ||
                  (dynamic_cast<UnfoldedIdentifierExpr *>((*comp)[1].get()) &&
                   (*comp)[0]->place_free()))) {
                _conds.emplace_back(ptr);
                return;
            }

            std::vector<Condition_ptr> cnds{ptr};
            auto cmp =
                std::make_shared<CompareConjunction>(cnds, std::is_same<T, OrCondition>::value);
            try_merge<T>(_conds, cmp, aggressive);
        }
    } else if (auto conj = std::dynamic_pointer_cast<CompareConjunction>(ptr)) {
        if ((std::is_same<T, OrCondition>::value && (conj->isNegated() || conj->singular())) ||
            (std::is_same<T, AndCondition>::value && (!conj->isNegated() || conj->singular()))) {
            if (auto lc = std::dynamic_pointer_cast<CompareConjunction>(
                    _conds.size() == 0 ? nullptr : _conds[0])) {
                if (lc->isNegated() == std::is_same<T, OrCondition>::value) {
                    auto cpy = std::make_shared<CompareConjunction>(*lc);
                    cpy->merge(*conj);
                    _conds[0] = cpy;
                } else {
                    if (conj->isNegated() == std::is_same<T, OrCondition>::value)
                        _conds.insert(_conds.begin(), conj);
                    else {
                        auto next = std::make_shared<CompareConjunction>(
                            std::is_same<T, OrCondition>::value);
                        next->merge(*conj);
                        _conds.insert(_conds.begin(), next);
                    }
                }
            } else {
                _conds.insert(_conds.begin(), conj);
            }
        } else {
            _conds.emplace_back(ptr);
        }
    } else {
        _conds.emplace_back(ptr);
    }
}

template <typename T, bool K>
auto make_log(const std::vector<Condition_ptr> &conds, bool aggressive) -> Condition_ptr {
    if (conds.size() == 0)
        return BooleanCondition::getShared(K);
    if (conds.size() == 1)
        return conds[0];

    std::vector<Condition_ptr> cnds;
    for (auto &c : conds)
        try_merge<T>(cnds, c, aggressive);
    auto res = std::make_shared<T>(cnds);
    if (res->singular())
        return *res->begin();
    if (res->empty())
        return BooleanCondition::getShared(K);
    return res;
}

auto make_or(const std::vector<Condition_ptr> &cptr) -> Condition_ptr {
    return make_log<OrCondition, false>(cptr, true);
}
auto make_and(const std::vector<Condition_ptr> &cptr) -> Condition_ptr {
    return make_log<AndCondition, true>(cptr, true);
}
auto make_or(const Condition_ptr &a, const Condition_ptr &b) -> Condition_ptr {
    std::vector<Condition_ptr> cnds{a, b};
    return make_log<OrCondition, false>(cnds, true);
}
auto make_and(const Condition_ptr &a, const Condition_ptr &b) -> Condition_ptr {
    std::vector<Condition_ptr> cnds{a, b};
    return make_log<AndCondition, true>(cnds, true);
}

// CONSTANTS
Condition_ptr BooleanCondition::FALSE_CONSTANT = std::make_shared<BooleanCondition>(false);
Condition_ptr BooleanCondition::TRUE_CONSTANT = std::make_shared<BooleanCondition>(true);
Condition_ptr DeadlockCondition::DEADLOCK = std::make_shared<DeadlockCondition>();

auto BooleanCondition::getShared(bool val) -> Condition_ptr {
    if (val) {
        return TRUE_CONSTANT;
    } else {
        return FALSE_CONSTANT;
    }
}

/******************** To TAPAAL Query ********************/

void SimpleQuantifierCondition::to_tapaal_query(std::ostream &out,
                                                TAPAALConditionExportContext &context) const {
    out << op() << " ";
    _cond->to_tapaal_query(out, context);
}

void UntilCondition::to_tapaal_query(std::ostream &out,
                                     TAPAALConditionExportContext &context) const {
    out << op() << " (";
    _cond1->to_tapaal_query(out, context);
    out << " U ";
    _cond2->to_tapaal_query(out, context);
    out << ")";
}

void LogicalCondition::to_tapaal_query(std::ostream &out,
                                       TAPAALConditionExportContext &context) const {
    out << "(";
    _conds[0]->to_tapaal_query(out, context);
    for (size_t i = 1; i < _conds.size(); ++i) {
        out << " " << op() << " ";
        _conds[i]->to_tapaal_query(out, context);
    }
    out << ")";
}

void CompareConjunction::to_tapaal_query(std::ostream &out,
                                         TAPAALConditionExportContext &context) const {
    out << "(";
    if (_negated)
        out << "!";
    bool first = true;
    for (auto &c : _constraints) {
        if (!first)
            out << " and ";
        if (c._lower != 0)
            out << "(" << c._lower << " <= " << context._netName << "." << c._name << ")";
        if (c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max())
            out << " and ";
        if (c._lower != 0)
            out << "(" << c._upper << " >= " << context._netName << "." << c._name << ")";
        first = false;
    }
    out << ")";
}

void CompareCondition::to_tapaal_query(std::ostream &out,
                                       TAPAALConditionExportContext &context) const {
    // If <id> <op> <literal>
    QueryPrinter printer;
    if (_expr1->type() == Expr::IDENTIFIER_EXPR && _expr2->type() == Expr::LITERAL_EXPR) {
        out << " ( " << context._netName << ".";
        _expr1->visit(printer);
        out << " " << opTAPAAL() << " ";
        _expr2->visit(printer);
        out << " ) ";
        // If <literal> <op> <id>
    } else if (_expr2->type() == Expr::IDENTIFIER_EXPR && _expr1->type() == Expr::LITERAL_EXPR) {
        out << " ( ";
        _expr1->visit(printer);
        out << " " << sopTAPAAL() << " " << context._netName << ".";
        _expr2->visit(printer);
        out << " ) ";
    } else {
        context._failed = true;
        out << " false ";
    }
}

void NotEqualCondition::to_tapaal_query(std::ostream &out,
                                        TAPAALConditionExportContext &context) const {
    out << " !( ";
    CompareCondition::to_tapaal_query(out, context);
    out << " ) ";
}

void NotCondition::to_tapaal_query(std::ostream &out, TAPAALConditionExportContext &context) const {
    out << " !( ";
    _cond->to_tapaal_query(out, context);
    out << " ) ";
}

void BooleanCondition::to_tapaal_query(std::ostream &out, TAPAALConditionExportContext &) const {
    if (value)
        out << "true";
    else
        out << "false";
}

void DeadlockCondition::to_tapaal_query(std::ostream &out, TAPAALConditionExportContext &) const {
    out << "deadlock";
}

void UnfoldedUpperBoundsCondition::to_tapaal_query(std::ostream &out,
                                                   TAPAALConditionExportContext &) const {
    out << "bounds (";
    for (size_t i = 0; i < _places.size(); ++i) {
        if (i != 0)
            out << ", ";
        out << _places[i]._name;
    }
    out << ")";
}

/******************** opTAPAAL ********************/

auto EqualCondition::opTAPAAL() const -> std::string { return "="; }

auto NotEqualCondition::opTAPAAL() const -> std::string {
    return "=";
} // Handled with hack in NotEqualCondition::toTAPAALQuery

auto LessThanCondition::opTAPAAL() const -> std::string { return "<"; }

auto LessThanOrEqualCondition::opTAPAAL() const -> std::string { return "<="; }

auto EqualCondition::sopTAPAAL() const -> std::string { return "="; }

auto NotEqualCondition::sopTAPAAL() const -> std::string {
    return "=";
} // Handled with hack in NotEqualCondition::toTAPAALQuery

auto LessThanCondition::sopTAPAAL() const -> std::string { return ">="; }

auto LessThanOrEqualCondition::sopTAPAAL() const -> std::string { return ">"; }

/******************** Context Analysis ********************/

void NaryExpr::analyze(AnalysisContext &context) {
    for (auto &e : _exprs)
        e->analyze(context);
}

void CommutativeExpr::analyze(AnalysisContext &context) {
    for (auto &i : _ids) {
        AnalysisContext::resolution_result_t result = context.resolve(i.second);
        if (result._success) {
            i.first = result._offset;
        } else {
            ExprError error("Unable to resolve identifier \"" + i.second + "\"", i.second.length());
            context.report_error(error);
        }
    }
    NaryExpr::analyze(context);
    std::sort(_ids.begin(), _ids.end(), [](auto &a, auto &b) { return a.first < b.first; });
    std::sort(_exprs.begin(), _exprs.end(), [](auto &a, auto &b) {
        auto ida = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(a);
        auto idb = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(b);
        if (ida == nullptr)
            return false;
        if (ida && !idb)
            return true;
        return ida->offset() < idb->offset();
    });
}

void MinusExpr::analyze(AnalysisContext &context) { _expr->analyze(context); }

void LiteralExpr::analyze(AnalysisContext &) { return; }

auto get_place(AnalysisContext &context, const std::string &name) -> uint32_t {
    AnalysisContext::resolution_result_t result = context.resolve(name);
    if (result._success) {
        return result._offset;
    } else {
        ExprError error("Unable to resolve identifier \"" + name + "\"", name.length());
        context.report_error(error);
    }
    return -1;
}

auto generate_unfolded_identifier_expr(ColoredAnalysisContext &context,
                                       std::unordered_map<uint32_t, std::string> &names,
                                       uint32_t colorIndex) -> Expr_ptr {
    std::string &place = names[colorIndex];
    return std::make_shared<UnfoldedIdentifierExpr>(place, get_place(context, place));
}

void IdentifierExpr::analyze(AnalysisContext &context) {
    if (_compiled) {
        _compiled->analyze(context);
        return;
    }

    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        std::unordered_map<uint32_t, std::string> names;
        if (!coloredContext->resolve_place(_name, names)) {
            ExprError error("Unable to resolve colored identifier \"" + _name + "\"",
                            _name.length());
            coloredContext->report_error(error);
        }

        if (names.size() == 1) {
            _compiled =
                generate_unfolded_identifier_expr(*coloredContext, names, names.begin()->first);
        } else {
            std::vector<Expr_ptr> identifiers;
            identifiers.reserve(names.size());
            for (auto &unfoldedName : names) {
                identifiers.push_back(
                    generate_unfolded_identifier_expr(*coloredContext, names, unfoldedName.first));
            }
            _compiled = std::make_shared<PQL::PlusExpr>(std::move(identifiers));
        }
    } else {
        _compiled = std::make_shared<UnfoldedIdentifierExpr>(_name, get_place(context, _name));
    }
    _compiled->analyze(context);
}

void UnfoldedIdentifierExpr::analyze(AnalysisContext &context) {
    AnalysisContext::resolution_result_t result = context.resolve(_name);
    if (result._success) {
        _offsetInMarking = result._offset;
    } else {
        ExprError error("Unable to resolve identifier \"" + _name + "\"", _name.length());
        context.report_error(error);
    }
}

void SimpleQuantifierCondition::analyze(AnalysisContext &context) { _cond->analyze(context); }

void UntilCondition::analyze(AnalysisContext &context) {
    _cond1->analyze(context);
    _cond2->analyze(context);
}

void LogicalCondition::analyze(AnalysisContext &context) {
    for (auto &c : _conds)
        c->analyze(context);
}

void UnfoldedFireableCondition::_analyze(AnalysisContext &context) {
    std::vector<Condition_ptr> conds;
    AnalysisContext::resolution_result_t result = context.resolve(_name, false);
    if (!result._success) {
        ExprError error("Unable to resolve identifier \"" + _name + "\"", _name.length());
        context.report_error(error);
        return;
    }

    assert(_name.compare(context.net().transition_names()[result._offset]) == 0);
    auto preset = context.net().preset(result._offset);
    for (; preset.first != preset.second; ++preset.first) {
        assert(preset.first->_place != std::numeric_limits<uint32_t>::max());
        assert(preset.first->_place != -1);
        auto id = std::make_shared<UnfoldedIdentifierExpr>(
            context.net().place_names()[preset.first->_place], preset.first->_place);
        auto lit = std::make_shared<LiteralExpr>(preset.first->_tokens);

        if (!preset.first->_inhibitor) {
            conds.emplace_back(std::make_shared<LessThanOrEqualCondition>(lit, id));
        } else if (preset.first->_tokens > 0) {
            conds.emplace_back(std::make_shared<LessThanCondition>(id, lit));
        }
    }
    if (conds.size() == 1)
        _compiled = conds[0];
    else
        _compiled = std::make_shared<AndCondition>(conds);
    _compiled->analyze(context);
}

void FireableCondition::_analyze(AnalysisContext &context) {

    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        std::vector<std::string> names;
        if (!coloredContext->resolve_transition(_name, names)) {
            ExprError error("Unable to resolve colored identifier \"" + _name + "\"",
                            _name.length());
            coloredContext->report_error(error);
            return;
        }
        if (names.size() < 1) {
            // If the transition points to empty vector we know that it has
            // no legal bindings and can never fire
            _compiled = std::make_shared<BooleanCondition>(false);
            _compiled->analyze(context);
            return;
        }
        if (names.size() == 1) {
            _compiled = std::make_shared<UnfoldedFireableCondition>(names[0]);
        } else {
            std::vector<Condition_ptr> identifiers;
            identifiers.reserve(names.size());
            for (auto &unfoldedName : names) {
                identifiers.push_back(std::make_shared<UnfoldedFireableCondition>(unfoldedName));
            }
            _compiled = std::make_shared<OrCondition>(std::move(identifiers));
        }
    } else {
        _compiled = std::make_shared<UnfoldedFireableCondition>(_name);
    }
    _compiled->analyze(context);
}

void CompareConjunction::analyze(AnalysisContext &context) {
    for (auto &c : _constraints) {
        c._place = get_place(context, c._name);
        assert(c._place >= 0);
    }
    std::sort(std::begin(_constraints), std::end(_constraints));
}

void CompareCondition::analyze(AnalysisContext &context) {
    _expr1->analyze(context);
    _expr2->analyze(context);
}

void NotCondition::analyze(AnalysisContext &context) { _cond->analyze(context); }

void BooleanCondition::analyze(AnalysisContext &) {}

void DeadlockCondition::analyze(AnalysisContext &c) { c.set_has_deadlock(); }

void KSafeCondition::_analyze(AnalysisContext &context) {
    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    std::vector<Condition_ptr> k_safe;
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        for (auto &p : coloredContext->all_colored_place_names())
            k_safe.emplace_back(std::make_shared<LessThanOrEqualCondition>(
                std::make_shared<IdentifierExpr>(p.first), _bound));
    } else {
        for (auto &p : context.all_place_names())
            k_safe.emplace_back(std::make_shared<LessThanOrEqualCondition>(
                std::make_shared<UnfoldedIdentifierExpr>(p.first), _bound));
    }
    _compiled = std::make_shared<AGCondition>(std::make_shared<AndCondition>(std::move(k_safe)));
    _compiled->analyze(context);
}

void QuasiLivenessCondition::_analyze(AnalysisContext &context) {
    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    std::vector<Condition_ptr> quasi;
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        for (auto &n : coloredContext->all_colored_transition_names()) {
            std::vector<Condition_ptr> disj;
            for (auto &tn : n.second)
                disj.emplace_back(std::make_shared<UnfoldedFireableCondition>(tn));
            quasi.emplace_back(
                std::make_shared<EFCondition>(std::make_shared<OrCondition>(std::move(disj))));
        }
    } else {
        for (auto &n : context.all_transition_names()) {
            quasi.emplace_back(std::make_shared<EFCondition>(
                std::make_shared<UnfoldedFireableCondition>(n.first)));
        }
    }
    _compiled = std::make_shared<AndCondition>(std::move(quasi));
    _compiled->analyze(context);
}

void LivenessCondition::_analyze(AnalysisContext &context) {
    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    std::vector<Condition_ptr> liveness;
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        for (auto &n : coloredContext->all_colored_transition_names()) {
            std::vector<Condition_ptr> disj;
            for (auto &tn : n.second)
                disj.emplace_back(std::make_shared<UnfoldedFireableCondition>(tn));
            liveness.emplace_back(std::make_shared<AGCondition>(
                std::make_shared<EFCondition>(std::make_shared<OrCondition>(std::move(disj)))));
        }
    } else {
        for (auto &n : context.all_transition_names()) {
            liveness.emplace_back(std::make_shared<AGCondition>(std::make_shared<EFCondition>(
                std::make_shared<UnfoldedFireableCondition>(n.first))));
        }
    }
    _compiled = std::make_shared<AndCondition>(std::move(liveness));
    _compiled->analyze(context);
}

void StableMarkingCondition::_analyze(AnalysisContext &context) {
    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    std::vector<Condition_ptr> stable_check;
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        for (auto &cpn : coloredContext->all_colored_place_names()) {
            std::vector<Expr_ptr> sum;
            MarkVal init_marking = 0;
            for (auto &pn : cpn.second) {
                auto id = std::make_shared<UnfoldedIdentifierExpr>(pn.second);
                id->analyze(context);
                init_marking += context.net().initial(id->offset());
                sum.emplace_back(std::move(id));
            }
            stable_check.emplace_back(std::make_shared<AGCondition>(
                std::make_shared<EqualCondition>(std::make_shared<PlusExpr>(std::move(sum)),
                                                 std::make_shared<LiteralExpr>(init_marking))));
        }
    } else {
        size_t i = 0;
        for (auto &p : context.net().place_names()) {
            stable_check.emplace_back(
                std::make_shared<AGCondition>(std::make_shared<EqualCondition>(
                    std::make_shared<UnfoldedIdentifierExpr>(p, i),
                    std::make_shared<LiteralExpr>(context.net().initial(i)))));
            ++i;
        }
    }
    _compiled = std::make_shared<OrCondition>(std::move(stable_check));
    _compiled->analyze(context);
}

void UpperBoundsCondition::_analyze(AnalysisContext &context) {
    auto coloredContext = dynamic_cast<ColoredAnalysisContext *>(&context);
    if (coloredContext != nullptr && coloredContext->is_colored()) {
        std::vector<std::string> uplaces;
        for (auto &p : _places) {
            std::unordered_map<uint32_t, std::string> names;
            if (!coloredContext->resolve_place(p, names)) {
                ExprError error("Unable to resolve colored identifier \"" + p + "\"", p.length());
                coloredContext->report_error(error);
            }

            for (auto &id : names) {
                uplaces.push_back(names[id.first]);
            }
        }
        _compiled = std::make_shared<UnfoldedUpperBoundsCondition>(uplaces);
    } else {
        _compiled = std::make_shared<UnfoldedUpperBoundsCondition>(_places);
    }
    _compiled->analyze(context);
}

void UnfoldedUpperBoundsCondition::analyze(AnalysisContext &c) {
    for (auto &p : _places) {
        AnalysisContext::resolution_result_t result = c.resolve(p._name);
        if (result._success) {
            p._place = result._offset;
        } else {
            ExprError error("Unable to resolve identifier \"" + p._name + "\"", p._name.length());
            c.report_error(error);
        }
    }
    std::sort(_places.begin(), _places.end());
}

/******************** Evaluation ********************/

auto NaryExpr::evaluate(const EvaluationContext &context) -> int {
    int32_t r = pre_op(context);
    for (size_t i = 1; i < _exprs.size(); ++i) {
        r = apply(r, _exprs[i]->eval_and_set(context));
    }
    return r;
}

auto NaryExpr::pre_op(const EvaluationContext &context) const -> int32_t {
    return _exprs[0]->evaluate(context);
}

auto CommutativeExpr::pre_op(const EvaluationContext &context) const -> int32_t {
    int32_t res = _constant;
    for (auto &i : _ids)
        res = this->apply(res, context.marking()[i.first]);
    if (_exprs.size() > 0)
        res = this->apply(res, _exprs[0]->eval_and_set(context));
    return res;
}

auto CommutativeExpr::evaluate(const EvaluationContext &context) -> int {
    if (_exprs.size() == 0)
        return pre_op(context);
    return NaryExpr::evaluate(context);
}

auto MinusExpr::evaluate(const EvaluationContext &context) -> int {
    return -(_expr->evaluate(context));
}

auto LiteralExpr::evaluate(const EvaluationContext &) -> int { return _value; }

auto UnfoldedIdentifierExpr::evaluate(const EvaluationContext &context) -> int {
    assert(_offsetInMarking != -1);
    return context.marking()[_offsetInMarking];
}

auto SimpleQuantifierCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    return RUNKNOWN;
}

auto EGCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    if (_cond->evaluate(context) == RFALSE)
        return RFALSE;
    return RUNKNOWN;
}

auto AGCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    if (_cond->evaluate(context) == RFALSE)
        return RFALSE;
    return RUNKNOWN;
}

auto EFCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    if (_cond->evaluate(context) == RTRUE)
        return RTRUE;
    return RUNKNOWN;
}

auto AFCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    if (_cond->evaluate(context) == RTRUE)
        return RTRUE;
    return RUNKNOWN;
}

auto ACondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    // if (_cond->evaluate(context) == RFALSE) return RFALSE;
    return RUNKNOWN;
}

auto ECondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    // if (_cond->evaluate(context) == RTRUE) return RTRUE;
    return RUNKNOWN;
}

auto FCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    // if (_cond->evaluate(context) == RTRUE) return RTRUE;
    return RUNKNOWN;
}

auto GCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    // if (_cond->evaluate(context) == RFALSE) return RFALSE;
    return RUNKNOWN;
}

/*        Condition::result_e XCondition::evaluate(const EvaluationContext& context) {
            return _cond->evaluate(context);
        }*/

auto UntilCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    auto r2 = _cond2->evaluate(context);
    if (r2 != RFALSE)
        return r2;
    auto r1 = _cond1->evaluate(context);
    if (r1 == RFALSE) {
        return RFALSE;
    }
    return RUNKNOWN;
}

auto AndCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    auto res = RTRUE;
    for (auto &c : _conds) {
        auto r = c->evaluate(context);
        if (r == RFALSE)
            return RFALSE;
        else if (r == RUNKNOWN)
            res = RUNKNOWN;
    }
    return res;
}

auto OrCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    auto res = RFALSE;
    for (auto &c : _conds) {
        auto r = c->evaluate(context);
        if (r == RTRUE)
            return RTRUE;
        else if (r == RUNKNOWN)
            res = RUNKNOWN;
    }
    return res;
}

auto CompareConjunction::evaluate(const EvaluationContext &context) -> Condition::result_e {
    //            auto rres = _org->evaluate(context);
    bool res = true;
    for (auto &c : _constraints) {
        res = res && context.marking()[c._place] <= c._upper &&
              context.marking()[c._place] >= c._lower;
        if (!res)
            break;
    }
    return (_negated xor res) ? RTRUE : RFALSE;
}

auto CompareCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    int v1 = _expr1->evaluate(context);
    int v2 = _expr2->evaluate(context);
    return apply(v1, v2) ? RTRUE : RFALSE;
}

auto NotCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->evaluate(context);
    if (res != RUNKNOWN)
        return res == RFALSE ? RTRUE : RFALSE;
    return RUNKNOWN;
}

auto BooleanCondition::evaluate(const EvaluationContext &) -> Condition::result_e {
    return value ? RTRUE : RFALSE;
}

auto DeadlockCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    if (!context.net())
        return RFALSE;
    if (!context.net()->deadlocked(context.marking())) {
        return RFALSE;
    }
    return RTRUE;
}

auto UnfoldedUpperBoundsCondition::value(const MarkVal *marking) -> size_t {
    size_t tmp = 0;
    for (auto &p : _places) {
        auto val = marking[p._place];
        p._maxed_out = (p._max <= val);
        tmp += val;
    }
    return tmp;
}

auto UnfoldedUpperBoundsCondition::evaluate(const EvaluationContext &context) -> Condition::result_e {
    setUpperBound(value(context.marking()));
    return _max <= _bound ? RTRUE : RUNKNOWN;
}

/******************** Evaluation - save result ********************/
auto SimpleQuantifierCondition::eval_and_set(const EvaluationContext &context)
    -> Condition::result_e {
    return RUNKNOWN;
}

auto GCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RFALSE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto FCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RTRUE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto EGCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RFALSE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto AGCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RFALSE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto EFCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RTRUE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto AFCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RTRUE)
        res = RUNKNOWN;
    set_satisfied(res);
    return res;
}

auto UntilCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto r2 = _cond2->eval_and_set(context);
    if (r2 != RFALSE)
        return r2;
    auto r1 = _cond1->eval_and_set(context);
    if (r1 == RFALSE)
        return RFALSE;
    return RUNKNOWN;
}

auto Expr::eval_and_set(const EvaluationContext &context) -> int {
    int r = evaluate(context);
    set_eval(r);
    return r;
}

auto AndCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    result_e res = RTRUE;
    for (auto &c : _conds) {
        auto r = c->eval_and_set(context);
        if (r == RFALSE) {
            res = RFALSE;
            break;
        } else if (r == RUNKNOWN) {
            res = RUNKNOWN;
        }
    }
    set_satisfied(res);
    return res;
}

auto OrCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    result_e res = RFALSE;
    for (auto &c : _conds) {
        auto r = c->eval_and_set(context);
        if (r == RTRUE) {
            res = RTRUE;
            break;
        } else if (r == RUNKNOWN) {
            res = RUNKNOWN;
        }
    }
    set_satisfied(res);
    return res;
}

auto CompareConjunction::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = evaluate(context);
    set_satisfied(res);
    return res;
}

auto CompareCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    int v1 = _expr1->eval_and_set(context);
    int v2 = _expr2->eval_and_set(context);
    bool res = apply(v1, v2);
    set_satisfied(res);
    return res ? RTRUE : RFALSE;
}

auto NotCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    auto res = _cond->eval_and_set(context);
    if (res != RUNKNOWN)
        res = res == RFALSE ? RTRUE : RFALSE;
    set_satisfied(res);
    return res;
}

auto BooleanCondition::eval_and_set(const EvaluationContext &) -> Condition::result_e {
    set_satisfied(value);
    return value ? RTRUE : RFALSE;
}

auto DeadlockCondition::eval_and_set(const EvaluationContext &context) -> Condition::result_e {
    if (!context.net())
        return RFALSE;
    set_satisfied(context.net()->deadlocked(context.marking()));
    return is_satisfied() ? RTRUE : RFALSE;
}

auto UnfoldedUpperBoundsCondition::eval_and_set(const EvaluationContext &context)
    -> Condition::result_e {
    auto res = evaluate(context);
    set_satisfied(res);
    return res;
}

/******************** Range Contexts ********************/

void UntilCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void EGCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void EUCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void EXCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void EFCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void AUCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void AXCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void AFCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void AGCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void ACondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void ECondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void GCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void FCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void XCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void AndCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void OrCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void NotCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void EqualCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void NotEqualCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void CompareConjunction::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void LessThanOrEqualCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void LessThanCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void BooleanCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void DeadlockCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void StableMarkingCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void QuasiLivenessCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void KSafeCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void LivenessCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void FireableCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UpperBoundsCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UnfoldedFireableCondition::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UnfoldedUpperBoundsCondition::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void LiteralExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void IdentifierExpr::visit(Visitor &ctx) const {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UnfoldedIdentifierExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void MinusExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void SubtractExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void PlusExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

void MultiplyExpr::visit(Visitor &ctx) const { ctx.accept<decltype(this)>(this); }

/******************** Mutating visitor **********************************/

void UntilCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void EGCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void EUCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void EXCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void EFCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void AUCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void AXCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void AFCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void AGCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void ACondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void ECondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void GCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void FCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void XCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void AndCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void OrCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void NotCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void EqualCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void NotEqualCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void CompareConjunction::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void LessThanOrEqualCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void LessThanCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void BooleanCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void DeadlockCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

void StableMarkingCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void QuasiLivenessCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void KSafeCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void LivenessCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void FireableCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UpperBoundsCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UnfoldedFireableCondition::visit(MutatingVisitor &ctx) {
    if (_compiled)
        _compiled->visit(ctx);
    else
        ctx.accept<decltype(this)>(this);
}

void UnfoldedUpperBoundsCondition::visit(MutatingVisitor &ctx) { ctx.accept<decltype(this)>(this); }

/******************** Apply (BinaryExpr subclasses) ********************/

auto PlusExpr::apply(int v1, int v2) const -> int { return v1 + v2; }

auto SubtractExpr::apply(int v1, int v2) const -> int { return v1 - v2; }

auto MultiplyExpr::apply(int v1, int v2) const -> int { return v1 * v2; }

/******************** Apply (CompareCondition subclasses) ********************/

auto EqualCondition::apply(int v1, int v2) const -> bool { return v1 == v2; }

auto NotEqualCondition::apply(int v1, int v2) const -> bool { return v1 != v2; }

auto LessThanCondition::apply(int v1, int v2) const -> bool { return v1 < v2; }

auto LessThanOrEqualCondition::apply(int v1, int v2) const -> bool { return v1 <= v2; }

/******************** Op (BinaryExpr subclasses) ********************/

auto PlusExpr::op() const -> std::string { return "+"; }

auto SubtractExpr::op() const -> std::string { return "-"; }

auto MultiplyExpr::op() const -> std::string { return "*"; }

/******************** Op (QuantifierCondition subclasses) ********************/

auto ACondition::op() const -> std::string { return "A"; }

auto ECondition::op() const -> std::string { return "E"; }

auto GCondition::op() const -> std::string { return "G"; }

auto FCondition::op() const -> std::string { return "F"; }

auto XCondition::op() const -> std::string { return "X"; }

auto EXCondition::op() const -> std::string { return "EX"; }

auto EGCondition::op() const -> std::string { return "EG"; }

auto EFCondition::op() const -> std::string { return "EF"; }

auto AXCondition::op() const -> std::string { return "AX"; }

auto AGCondition::op() const -> std::string { return "AG"; }

auto AFCondition::op() const -> std::string { return "AF"; }

/******************** Op (UntilCondition subclasses) ********************/

auto UntilCondition::op() const -> std::string { return ""; }

auto EUCondition::op() const -> std::string { return "E"; }

auto AUCondition::op() const -> std::string { return "A"; }

/******************** Op (LogicalCondition subclasses) ********************/

auto AndCondition::op() const -> std::string { return "and"; }

auto OrCondition::op() const -> std::string { return "or"; }

/******************** Op (CompareCondition subclasses) ********************/

auto EqualCondition::op() const -> std::string { return "=="; }

auto NotEqualCondition::op() const -> std::string { return "!="; }

auto LessThanCondition::op() const -> std::string { return "<"; }

auto LessThanOrEqualCondition::op() const -> std::string { return "<="; }

/******************** free of places ********************/

auto NaryExpr::place_free() const -> bool {
    for (auto &e : _exprs)
        if (!e->place_free())
            return false;
    return true;
}

auto CommutativeExpr::place_free() const -> bool {
    if (_ids.size() > 0)
        return false;
    return NaryExpr::place_free();
}

auto MinusExpr::place_free() const -> bool { return _expr->place_free(); }

/******************** Expr::type() implementation ********************/

auto PlusExpr::type() const -> Expr::types_e { return Expr::PLUS_EXPR; }

auto SubtractExpr::type() const -> Expr::types_e { return Expr::SUBTRACT_EXPR; }

auto MultiplyExpr::type() const -> Expr::types_e { return Expr::MULTIPLY_EXPR; }

auto MinusExpr::type() const -> Expr::types_e { return Expr::MINUS_EXPR; }

auto LiteralExpr::type() const -> Expr::types_e { return Expr::LITERAL_EXPR; }

auto UnfoldedIdentifierExpr::type() const -> Expr::types_e { return Expr::IDENTIFIER_EXPR; }

/******************** Distance Condition ********************/

template <> auto delta<EqualCondition>(int v1, int v2, bool negated) -> uint32_t {
    if (!negated)
        return std::abs(v1 - v2);
    else
        return v1 == v2 ? 1 : 0;
}

template <> auto delta<NotEqualCondition>(int v1, int v2, bool negated) -> uint32_t {
    return delta<EqualCondition>(v1, v2, !negated);
}

template <> auto delta<LessThanCondition>(int v1, int v2, bool negated) -> uint32_t {
    if (!negated)
        return v1 < v2 ? 0 : v1 - v2 + 1;
    else
        return v1 >= v2 ? 0 : v2 - v1;
}

template <> auto delta<LessThanOrEqualCondition>(int v1, int v2, bool negated) -> uint32_t {
    if (!negated)
        return v1 <= v2 ? 0 : v1 - v2;
    else
        return v1 > v2 ? 0 : v2 - v1 + 1;
}

auto NotCondition::distance(DistanceContext &context) const -> uint32_t {
    context.negate();
    uint32_t retval = _cond->distance(context);
    context.negate();
    return retval;
}

auto BooleanCondition::distance(DistanceContext &context) const -> uint32_t {
    if (context.negated() != value)
        return 0;
    return std::numeric_limits<uint32_t>::max();
}

auto DeadlockCondition::distance(DistanceContext &context) const -> uint32_t { return 0; }

auto UnfoldedUpperBoundsCondition::distance(DistanceContext &context) const -> uint32_t {
    size_t tmp = 0;
    for (auto &p : _places) {
        tmp += context.marking()[p._place];
    }

    return _max - tmp;
}

auto EFCondition::distance(DistanceContext &context) const -> uint32_t {
    return _cond->distance(context);
}

auto EGCondition::distance(DistanceContext &context) const -> uint32_t {
    return _cond->distance(context);
}

auto EXCondition::distance(DistanceContext &context) const -> uint32_t {
    return _cond->distance(context);
}

auto EUCondition::distance(DistanceContext &context) const -> uint32_t {
    return _cond2->distance(context);
}

auto AFCondition::distance(DistanceContext &context) const -> uint32_t {
    context.negate();
    uint32_t retval = _cond->distance(context);
    context.negate();
    return retval;
}

auto AXCondition::distance(DistanceContext &context) const -> uint32_t {
    context.negate();
    uint32_t retval = _cond->distance(context);
    context.negate();
    return retval;
}

auto AGCondition::distance(DistanceContext &context) const -> uint32_t {
    context.negate();
    uint32_t retval = _cond->distance(context);
    context.negate();
    return retval;
}

auto AUCondition::distance(DistanceContext &context) const -> uint32_t {
    context.negate();
    auto r1 = _cond1->distance(context);
    auto r2 = _cond2->distance(context);
    context.negate();
    return r1 + r2;
}

auto CompareConjunction::distance(DistanceContext &context) const -> uint32_t {
    uint32_t d = 0;
    auto neg = context.negated() != _negated;
    if (!neg) {
        for (auto &c : _constraints) {
            auto pv = context.marking()[c._place];
            d += (c._upper == std::numeric_limits<uint32_t>::max()
                      ? 0
                      : delta<LessThanOrEqualCondition>(pv, c._upper, neg)) +
                 (c._lower == 0 ? 0 : delta<LessThanOrEqualCondition>(c._lower, pv, neg));
        }
    } else {
        bool first = true;
        for (auto &c : _constraints) {
            auto pv = context.marking()[c._place];
            if (c._upper != std::numeric_limits<uint32_t>::max()) {
                auto d2 = delta<LessThanOrEqualCondition>(pv, c._upper, neg);
                if (first)
                    d = d2;
                else
                    d = std::min(d, d2);
                first = false;
            }

            if (c._lower != 0) {
                auto d2 = delta<LessThanOrEqualCondition>(c._upper, pv, neg);
                if (first)
                    d = d2;
                else
                    d = std::min(d, d2);
                first = false;
            }
        }
    }
    return d;
}

auto conj_distance(DistanceContext &context, const std::vector<Condition_ptr> &conds) -> uint32_t {
    uint32_t val = 0;
    for (auto &c : conds)
        val += c->distance(context);
    return val;
}

auto disj_distance(DistanceContext &context, const std::vector<Condition_ptr> &conds) -> uint32_t {
    uint32_t val = std::numeric_limits<uint32_t>::max();
    for (auto &c : conds)
        val = std::min(c->distance(context), val);
    return val;
}

auto AndCondition::distance(DistanceContext &context) const -> uint32_t {
    if (context.negated())
        return disj_distance(context, _conds);
    else
        return conj_distance(context, _conds);
}

auto OrCondition::distance(DistanceContext &context) const -> uint32_t {
    if (context.negated())
        return conj_distance(context, _conds);
    else
        return disj_distance(context, _conds);
}

struct s_t {
    int _d;
    unsigned int _p;
};

auto LessThanOrEqualCondition::distance(DistanceContext &context) const -> uint32_t {
    return _distance(context, delta<LessThanOrEqualCondition>);
}

auto LessThanCondition::distance(DistanceContext &context) const -> uint32_t {
    return _distance(context, delta<LessThanCondition>);
}

auto NotEqualCondition::distance(DistanceContext &context) const -> uint32_t {
    return _distance(context, delta<NotEqualCondition>);
}

auto EqualCondition::distance(DistanceContext &context) const -> uint32_t {
    return _distance(context, delta<EqualCondition>);
}

/******************** BIN output ********************/

void LiteralExpr::to_binary(std::ostream &out) const {
    out.write("l", sizeof(char));
    out.write(reinterpret_cast<const char *>(&_value), sizeof(int));
}

void UnfoldedIdentifierExpr::to_binary(std::ostream &out) const {
    out.write("i", sizeof(char));
    out.write(reinterpret_cast<const char *>(&_offsetInMarking), sizeof(int));
}

void MinusExpr::to_binary(std::ostream &out) const {
    auto e1 = std::make_shared<PQL::LiteralExpr>(0);
    std::vector<Expr_ptr> exprs;
    exprs.push_back(e1);
    exprs.push_back(_expr);
    PQL::SubtractExpr(std::move(exprs)).to_binary(out);
}

void SubtractExpr::to_binary(std::ostream &out) const {
    out.write("-", sizeof(char));
    uint32_t size = _exprs.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    for (auto &e : _exprs)
        e->to_binary(out);
}

void CommutativeExpr::to_binary(std::ostream &out) const {
    auto sop = op();
    out.write(&sop[0], sizeof(char));
    out.write(reinterpret_cast<const char *>(&_constant), sizeof(int32_t));
    uint32_t size = _ids.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    size = _exprs.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    for (auto &id : _ids)
        out.write(reinterpret_cast<const char *>(&id.first), sizeof(uint32_t));
    for (auto &e : _exprs)
        e->to_binary(out);
}

void SimpleQuantifierCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = get_quantifier();
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    _cond->to_binary(out);
}

void UntilCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = get_quantifier();
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    _cond1->to_binary(out);
    _cond2->to_binary(out);
}

void LogicalCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = get_quantifier();
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    uint32_t size = _conds.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    for (auto &c : _conds)
        c->to_binary(out);
}

void CompareConjunction::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = quantifier_e::COMPCONJ;
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    out.write(reinterpret_cast<const char *>(&_negated), sizeof(bool));
    uint32_t size = _constraints.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    for (auto &c : _constraints) {
        out.write(reinterpret_cast<const char *>(&c._place), sizeof(int32_t));
        out.write(reinterpret_cast<const char *>(&c._lower), sizeof(uint32_t));
        out.write(reinterpret_cast<const char *>(&c._upper), sizeof(uint32_t));
    }
}

void CompareCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = get_quantifier();
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    std::string sop = op();
    out.write(sop.data(), sop.size());
    out.write("\0", sizeof(char));
    _expr1->to_binary(out);
    _expr2->to_binary(out);
}

void DeadlockCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = quantifier_e::DEADLOCK;
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
}

void BooleanCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = quantifier_e::PN_BOOLEAN;
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    out.write(reinterpret_cast<const char *>(&value), sizeof(bool));
}

void UnfoldedUpperBoundsCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = quantifier_e::UPPERBOUNDS;
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    uint32_t size = _places.size();
    out.write(reinterpret_cast<const char *>(&size), sizeof(uint32_t));
    out.write(reinterpret_cast<const char *>(&_max), sizeof(double));
    out.write(reinterpret_cast<const char *>(&_offset), sizeof(double));
    for (auto &b : _places) {
        out.write(reinterpret_cast<const char *>(&b._place), sizeof(uint32_t));
        out.write(reinterpret_cast<const char *>(&b._max), sizeof(double));
    }
}

void NotCondition::to_binary(std::ostream &out) const {
    auto path = get_path();
    auto quant = get_quantifier();
    out.write(reinterpret_cast<const char *>(&path), sizeof(path_e));
    out.write(reinterpret_cast<const char *>(&quant), sizeof(quantifier_e));
    _cond->to_binary(out);
}

/******************** CTL Output ********************/

void LiteralExpr::to_xml(std::ostream &out, uint32_t tabs, bool tokencount) const {
    generate_tabs(out, tabs) << "<integer-constant>" + std::to_string(_value) +
                                    "</integer-constant>\n";
}

void UnfoldedFireableCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<is-fireable><transition>" + _name
                             << "</transition></is-fireable>\n";
}

void UnfoldedIdentifierExpr::to_xml(std::ostream &out, uint32_t tabs, bool tokencount) const {
    if (tokencount) {
        generate_tabs(out, tabs) << "<place>" << _name << "</place>\n";
    } else {
        generate_tabs(out, tabs) << "<tokens-count>\n";
        generate_tabs(out, tabs + 1) << "<place>" << _name << "</place>\n";
        generate_tabs(out, tabs) << "</tokens-count>\n";
    }
}

void PlusExpr::to_xml(std::ostream &ss, uint32_t tabs, bool tokencount) const {
    if (tokencount) {
        for (auto &e : _exprs)
            e->to_xml(ss, tabs, tokencount);
        return;
    }

    if (_tk) {
        generate_tabs(ss, tabs) << "<tokens-count>\n";
        for (auto &e : _ids)
            generate_tabs(ss, tabs + 1) << "<place>" << e.second << "</place>\n";
        for (auto &e : _exprs)
            e->to_xml(ss, tabs + 1, true);
        generate_tabs(ss, tabs) << "</tokens-count>\n";
        return;
    }
    generate_tabs(ss, tabs) << "<integer-sum>\n";
    generate_tabs(ss, tabs + 1) << "<integer-constant>" + std::to_string(_constant) +
                                       "</integer-constant>\n";
    for (auto &i : _ids) {
        generate_tabs(ss, tabs + 1) << "<tokens-count>\n";
        generate_tabs(ss, tabs + 2) << "<place>" << i.second << "</place>\n";
        generate_tabs(ss, tabs + 1) << "</tokens-count>\n";
    }
    for (auto &e : _exprs)
        e->to_xml(ss, tabs + 1, tokencount);
    generate_tabs(ss, tabs) << "</integer-sum>\n";
}

void SubtractExpr::to_xml(std::ostream &ss, uint32_t tabs, bool tokencount) const {
    generate_tabs(ss, tabs) << "<integer-difference>\n";
    for (auto &e : _exprs)
        e->to_xml(ss, tabs + 1);
    generate_tabs(ss, tabs) << "</integer-difference>\n";
}

void MultiplyExpr::to_xml(std::ostream &ss, uint32_t tabs, bool tokencount) const {
    generate_tabs(ss, tabs) << "<integer-product>\n";
    for (auto &e : _exprs)
        e->to_xml(ss, tabs + 1);
    generate_tabs(ss, tabs) << "</integer-product>\n";
}

void MinusExpr::to_xml(std::ostream &out, uint32_t tabs, bool tokencount) const {

    generate_tabs(out, tabs) << "<integer-product>\n";
    _expr->to_xml(out, tabs + 1);
    generate_tabs(out, tabs + 1) << "<integer-difference>\n";
    generate_tabs(out, tabs + 2) << "<integer-constant>0</integer-constant>\n";
    generate_tabs(out, tabs + 2) << "<integer-constant>1</integer-constant>\n";
    generate_tabs(out, tabs + 1) << "</integer-difference>\n";
    generate_tabs(out, tabs) << "</integer-product>\n";
}

void EXCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<exists-path>\n";
    generate_tabs(out, tabs + 1) << "<next>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</next>\n";
    generate_tabs(out, tabs) << "</exists-path>\n";
}

void AXCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<all-paths>\n";
    generate_tabs(out, tabs + 1) << "<next>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</next>\n";
    generate_tabs(out, tabs) << "</all-paths>\n";
}

void EFCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<exists-path>\n";
    generate_tabs(out, tabs + 1) << "<finally>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</finally>\n";
    generate_tabs(out, tabs) << "</exists-path>\n";
}

void AFCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<all-paths>\n";
    generate_tabs(out, tabs + 1) << "<finally>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</finally>\n";
    generate_tabs(out, tabs) << "</all-paths>\n";
}

void EGCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<exists-path>\n";
    generate_tabs(out, tabs + 1) << "<globally>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</globally>\n";
    generate_tabs(out, tabs) << "</exists-path>\n";
}

void AGCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<all-paths>\n";
    generate_tabs(out, tabs + 1) << "<globally>\n";
    _cond->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</globally>\n";
    generate_tabs(out, tabs) << "</all-paths>\n";
}

void EUCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<exists-path>\n";
    generate_tabs(out, tabs + 1) << "<until>\n";
    generate_tabs(out, tabs + 2) << "<before>\n";
    _cond1->to_xml(out, tabs + 3);
    generate_tabs(out, tabs + 2) << "</before>\n";
    generate_tabs(out, tabs + 2) << "<reach>\n";
    _cond2->to_xml(out, tabs + 3);
    generate_tabs(out, tabs + 2) << "</reach>\n";
    generate_tabs(out, tabs + 1) << "</until>\n";
    generate_tabs(out, tabs) << "</exists-path>\n";
}

void AUCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<all-paths>\n";
    generate_tabs(out, tabs + 1) << "<until>\n";
    generate_tabs(out, tabs + 2) << "<before>\n";
    _cond1->to_xml(out, tabs + 3);
    generate_tabs(out, tabs + 2) << "</before>\n";
    generate_tabs(out, tabs + 2) << "<reach>\n";
    _cond2->to_xml(out, tabs + 3);
    generate_tabs(out, tabs + 2) << "</reach>\n";
    generate_tabs(out, tabs + 1) << "</until>\n";
    generate_tabs(out, tabs) << "</all-paths>\n";
}

void ACondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<all-paths>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</all-paths>\n";
}

void ECondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<exists-path>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</exists-path>\n";
}

void FCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<finally>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</finally>\n";
}

void GCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<globally>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</globally>\n";
}

void XCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<next>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</next>\n";
}

void UntilCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<until>\n";
    generate_tabs(out, tabs + 1) << "<before>\n";
    _cond1->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</before>\n";
    generate_tabs(out, tabs + 1) << "<reach>\n";
    _cond2->to_xml(out, tabs + 2);
    generate_tabs(out, tabs + 1) << "</reach>\n";
    generate_tabs(out, tabs) << "</until>\n";
}

void AndCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    if (_conds.size() == 0) {
        BooleanCondition::TRUE_CONSTANT->to_xml(out, tabs);
        return;
    }
    if (_conds.size() == 1) {
        _conds[0]->to_xml(out, tabs);
        return;
    }
    generate_tabs(out, tabs) << "<conjunction>\n";
    _conds[0]->to_xml(out, tabs + 1);
    for (size_t i = 1; i < _conds.size(); ++i) {
        if (i + 1 == _conds.size()) {
            _conds[i]->to_xml(out, tabs + i + 1);
        } else {
            generate_tabs(out, tabs + i) << "<conjunction>\n";
            _conds[i]->to_xml(out, tabs + i + 1);
        }
    }
    for (size_t i = _conds.size() - 1; i > 1; --i) {
        generate_tabs(out, tabs + i) << "</conjunction>\n";
    }
    generate_tabs(out, tabs) << "</conjunction>\n";
}

void OrCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    if (_conds.size() == 0) {
        BooleanCondition::FALSE_CONSTANT->to_xml(out, tabs);
        return;
    }
    if (_conds.size() == 1) {
        _conds[0]->to_xml(out, tabs);
        return;
    }
    generate_tabs(out, tabs) << "<disjunction>\n";
    _conds[0]->to_xml(out, tabs + 1);
    for (size_t i = 1; i < _conds.size(); ++i) {
        if (i + 1 == _conds.size()) {
            _conds[i]->to_xml(out, tabs + i + 1);
        } else {
            generate_tabs(out, tabs + i) << "<disjunction>\n";
            _conds[i]->to_xml(out, tabs + i + 1);
        }
    }
    for (size_t i = _conds.size() - 1; i > 1; --i) {
        generate_tabs(out, tabs + i) << "</disjunction>\n";
    }
    generate_tabs(out, tabs) << "</disjunction>\n";
}

void CompareConjunction::to_xml(std::ostream &out, uint32_t tabs) const {
    if (_negated)
        generate_tabs(out, tabs++) << "<negation>";
    if (_constraints.size() == 0)
        BooleanCondition::TRUE_CONSTANT->to_xml(out, tabs);
    else {
        bool single = _constraints.size() == 1 &&
                      (_constraints[0]._lower == 0 ||
                       _constraints[0]._upper == std::numeric_limits<uint32_t>::max());
        if (!single)
            generate_tabs(out, tabs) << "<conjunction>\n";
        for (auto &c : _constraints) {
            if (c._lower != 0) {
                generate_tabs(out, tabs + 1) << "<integer-ge>\n";
                generate_tabs(out, tabs + 2) << "<tokens-count>\n";
                generate_tabs(out, tabs + 3) << "<place>" << c._name << "</place>\n";
                generate_tabs(out, tabs + 2) << "</tokens-count>\n";
                generate_tabs(out, tabs + 2)
                    << "<integer-constant>" << c._lower << "</integer-constant>\n";
                generate_tabs(out, tabs + 1) << "</integer-ge>\n";
            }
            if (c._upper != std::numeric_limits<uint32_t>::max()) {
                generate_tabs(out, tabs + 1) << "<integer-le>\n";
                generate_tabs(out, tabs + 2) << "<tokens-count>\n";
                generate_tabs(out, tabs + 3) << "<place>" << c._name << "</place>\n";
                generate_tabs(out, tabs + 2) << "</tokens-count>\n";
                generate_tabs(out, tabs + 2)
                    << "<integer-constant>" << c._upper << "</integer-constant>\n";
                generate_tabs(out, tabs + 1) << "</integer-le>\n";
            }
        }
        if (!single)
            generate_tabs(out, tabs) << "</conjunction>\n";
    }
    if (_negated)
        generate_tabs(out, --tabs) << "</negation>";
}

void EqualCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<integer-eq>\n";
    _expr1->to_xml(out, tabs + 1);
    _expr2->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</integer-eq>\n";
}

void NotEqualCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<integer-ne>\n";
    _expr1->to_xml(out, tabs + 1);
    _expr2->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</integer-ne>\n";
}

void LessThanCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<integer-lt>\n";
    _expr1->to_xml(out, tabs + 1);
    _expr2->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</integer-lt>\n";
}

void LessThanOrEqualCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<integer-le>\n";
    _expr1->to_xml(out, tabs + 1);
    _expr2->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</integer-le>\n";
}

void NotCondition::to_xml(std::ostream &out, uint32_t tabs) const {

    generate_tabs(out, tabs) << "<negation>\n";
    _cond->to_xml(out, tabs + 1);
    generate_tabs(out, tabs) << "</negation>\n";
}

void BooleanCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<" << (value ? "true" : "false") << "/>\n";
}

void DeadlockCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<deadlock/>\n";
}

void UnfoldedUpperBoundsCondition::to_xml(std::ostream &out, uint32_t tabs) const {
    generate_tabs(out, tabs) << "<place-bound>\n";
    for (auto &p : _places)
        generate_tabs(out, tabs + 1) << "<place>" << p._name << "</place>\n";
    generate_tabs(out, tabs) << "</place-bound>\n";
}

/******************** Query Simplification ********************/

auto LiteralExpr::constraint(SimplificationContext &context) const -> Member {
    return Member(_value);
}

auto member_for_place(size_t p, SimplificationContext &context) -> Member {
    std::vector<int> row(context.net().number_of_transitions(), 0);
    row.shrink_to_fit();
    for (size_t t = 0; t < context.net().number_of_transitions(); t++) {
        row[t] = context.net().out_arc(t, p) - context.net().in_arc(p, t);
    }
    return Member(std::move(row), context.marking()[p]);
}

auto UnfoldedIdentifierExpr::constraint(SimplificationContext &context) const -> Member {
    return member_for_place(_offsetInMarking, context);
}

auto CommutativeExpr::commutative_cons(int constant, SimplificationContext &context,
                                       const std::function<void(Member &a, Member b)> &op) const
    -> Member {
    Member res;
    bool first = true;
    if (_constant != constant || (_exprs.size() == 0 && _ids.size() == 0)) {
        first = false;
        res = Member(_constant);
    }

    for (auto &i : _ids) {
        if (first)
            res = member_for_place(i.first, context);
        else
            op(res, member_for_place(i.first, context));
        first = false;
    }

    for (auto &e : _exprs) {
        if (first)
            res = e->constraint(context);
        else
            op(res, e->constraint(context));
        first = false;
    }
    return res;
}

auto PlusExpr::constraint(SimplificationContext &context) const -> Member {
    return commutative_cons(0, context, [](auto &a, auto b) { a += b; });
}

auto SubtractExpr::constraint(SimplificationContext &context) const -> Member {
    Member res = _exprs[0]->constraint(context);
    for (size_t i = 1; i < _exprs.size(); ++i)
        res -= _exprs[i]->constraint(context);
    return res;
}

auto MultiplyExpr::constraint(SimplificationContext &context) const -> Member {
    return commutative_cons(1, context, [](auto &a, auto b) { a *= b; });
}

auto MinusExpr::constraint(SimplificationContext &context) const -> Member {
    Member neg(-1);
    return _expr->constraint(context) *= neg;
}

auto simplify_ex(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(std::make_shared<NotCondition>(std::make_shared<DeadlockCondition>()));
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<EXCondition>(r._formula));
    }
}

auto simplify_ax(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(std::make_shared<DeadlockCondition>());
    } else {
        return retval_t(std::make_shared<AXCondition>(r._formula));
    }
}

auto simplify_ef(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<EFCondition>(r._formula));
    }
}

auto simplify_af(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<AFCondition>(r._formula));
    }
}

auto simplify_eg(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<EGCondition>(r._formula));
    }
}

auto simplify_ag(retval_t &r, SimplificationContext &context) -> retval_t {
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<AGCondition>(r._formula));
    }
}

template <typename Quantifier>
auto simplify_simple_quant(retval_t &r, SimplificationContext &context) -> retval_t {
    static_assert(std::is_base_of_v<SimpleQuantifierCondition, Quantifier>);
    if (r._formula->is_trivially_true() || !r._neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r._formula->is_trivially_false() || !r._lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        return retval_t(std::make_shared<Quantifier>(r._formula));
    }
}

auto EXCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_ax(r, context) : simplify_ex(r, context);
}

auto AXCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_ex(r, context) : simplify_ax(r, context);
}

auto EFCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_ag(r, context) : simplify_ef(r, context);
}

auto AFCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_eg(r, context) : simplify_af(r, context);
}

auto EGCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_af(r, context) : simplify_eg(r, context);
}

auto AGCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_ef(r, context) : simplify_ag(r, context);
}

auto EUCondition::simplify(SimplificationContext &context) const -> retval_t {
    // cannot push negation any further
    bool neg = context.negated();
    context.set_negate(false);
    retval_t r2 = _cond2->simplify(context);
    if (r2._formula->is_trivially_true() || !r2._neglps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::FALSE_CONSTANT)
                   : retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r2._formula->is_trivially_false() || !r2._lps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::TRUE_CONSTANT)
                   : retval_t(BooleanCondition::FALSE_CONSTANT);
    }
    retval_t r1 = _cond1->simplify(context);
    context.set_negate(neg);

    if (context.negated()) {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(
                std::make_shared<NotCondition>(std::make_shared<EFCondition>(r2._formula)));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return retval_t(std::make_shared<NotCondition>(r2._formula));
        } else {
            return retval_t(std::make_shared<NotCondition>(
                std::make_shared<EUCondition>(r1._formula, r2._formula)));
        }
    } else {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(std::make_shared<EFCondition>(r2._formula));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return r2;
        } else {
            return retval_t(std::make_shared<EUCondition>(r1._formula, r2._formula));
        }
    }
}

auto AUCondition::simplify(SimplificationContext &context) const -> retval_t {
    // cannot push negation any further
    bool neg = context.negated();
    context.set_negate(false);
    retval_t r2 = _cond2->simplify(context);
    if (r2._formula->is_trivially_true() || !r2._neglps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::FALSE_CONSTANT)
                   : retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r2._formula->is_trivially_false() || !r2._lps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::TRUE_CONSTANT)
                   : retval_t(BooleanCondition::FALSE_CONSTANT);
    }
    retval_t r1 = _cond1->simplify(context);
    context.set_negate(neg);

    if (context.negated()) {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(
                std::make_shared<NotCondition>(std::make_shared<AFCondition>(r2._formula)));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return retval_t(std::make_shared<NotCondition>(r2._formula));
        } else {
            return retval_t(std::make_shared<NotCondition>(
                std::make_shared<AUCondition>(r1._formula, r2._formula)));
        }
    } else {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(std::make_shared<AFCondition>(r2._formula));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return r2;
        } else {
            return retval_t(std::make_shared<AUCondition>(r1._formula, r2._formula));
        }
    }
}

auto UntilCondition::simplify(SimplificationContext &context) const -> retval_t {
    bool neg = context.negated();
    context.set_negate(false);

    retval_t r2 = _cond2->simplify(context);
    if (r2._formula->is_trivially_true() || !r2._neglps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::FALSE_CONSTANT)
                   : retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (r2._formula->is_trivially_false() || !r2._lps->satisfiable(context)) {
        context.set_negate(neg);
        return neg ? retval_t(BooleanCondition::TRUE_CONSTANT)
                   : retval_t(BooleanCondition::FALSE_CONSTANT);
    }
    retval_t r1 = _cond1->simplify(context);
    context.set_negate(neg);

    if (context.negated()) {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(
                std::make_shared<NotCondition>(std::make_shared<FCondition>(r2._formula)));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return retval_t(std::make_shared<NotCondition>(r2._formula));
        } else {
            return retval_t(std::make_shared<NotCondition>(
                std::make_shared<UntilCondition>(r1._formula, r2._formula)));
        }
    } else {
        if (r1._formula->is_trivially_true() || !r1._neglps->satisfiable(context)) {
            return retval_t(std::make_shared<FCondition>(r2._formula));
        } else if (r1._formula->is_trivially_false() || !r1._lps->satisfiable(context)) {
            return r2;
        } else {
            return retval_t(std::make_shared<UntilCondition>(r1._formula, r2._formula));
        }
    }
}

auto ECondition::simplify(SimplificationContext &context) const -> retval_t {
    assert(false);
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_simple_quant<ACondition>(r, context)
                             : simplify_simple_quant<ECondition>(r, context);
}

auto ACondition::simplify(SimplificationContext &context) const -> retval_t {
    assert(false);
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_simple_quant<ECondition>(r, context)
                             : simplify_simple_quant<ACondition>(r, context);
}

auto FCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_simple_quant<GCondition>(r, context)
                             : simplify_simple_quant<FCondition>(r, context);
}

auto GCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return context.negated() ? simplify_simple_quant<FCondition>(r, context)
                             : simplify_simple_quant<GCondition>(r, context);
}

auto XCondition::simplify(SimplificationContext &context) const -> retval_t {
    retval_t r = _cond->simplify(context);
    return simplify_simple_quant<XCondition>(r, context);
}

auto merge_lps(std::vector<AbstractProgramCollection_ptr> &&lps) -> AbstractProgramCollection_ptr {
    if (lps.empty())
        return nullptr;

    auto i = lps.size() - 1;
    decltype(i) j = 0;
    while (i > 0) {
        if (i <= j)
            j = 0;
        else {
            lps[j] = std::make_shared<MergeCollection>(lps[j], lps[i]);
            --i;
            ++j;
        }
    }
    return lps[0];
}

auto LogicalCondition::simplify_and(SimplificationContext &context) const -> retval_t {

    std::vector<Condition_ptr> conditions;
    std::vector<AbstractProgramCollection_ptr> lpsv;
    std::vector<AbstractProgramCollection_ptr> neglps;
    for (auto &c : _conds) {
        auto r = c->simplify(context);
        if (r._formula->is_trivially_false()) {
            return retval_t(BooleanCondition::FALSE_CONSTANT);
        } else if (r._formula->is_trivially_true()) {
            continue;
        }

        conditions.push_back(r._formula);
        lpsv.emplace_back(r._lps);
        neglps.emplace_back(r._neglps);
    }

    if (conditions.size() == 0) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    }

    auto lps = merge_lps(std::move(lpsv));

    try {
        if (!context.timeout() && !lps->satisfiable(context)) {
            return retval_t(BooleanCondition::FALSE_CONSTANT);
        }
    } catch (std::bad_alloc &e) {
        // we are out of memory, deal with it.
        std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
    }

    // Lets try to see if the r1 AND r2 can ever be false at the same time
    // If not, then we know that r1 || r2 must be true.
    // we check this by checking if !r1 && !r2 is unsat

    return retval_t(make_and(conditions), std::move(lps),
                  std::make_shared<UnionCollection>(std::move(neglps)));
}

auto LogicalCondition::simplifyOr(SimplificationContext &context) const -> retval_t {

    std::vector<Condition_ptr> conditions;
    std::vector<AbstractProgramCollection_ptr> lps, neglpsv;
    for (auto &c : _conds) {
        auto r = c->simplify(context);
        assert(r._neglps);
        assert(r._lps);

        if (r._formula->is_trivially_true()) {
            return retval_t(BooleanCondition::TRUE_CONSTANT);
        } else if (r._formula->is_trivially_false()) {
            continue;
        }
        conditions.push_back(r._formula);
        lps.push_back(r._lps);
        neglpsv.emplace_back(r._neglps);
    }

    AbstractProgramCollection_ptr neglps = merge_lps(std::move(neglpsv));

    if (conditions.size() == 0) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    }

    try {
        if (!context.timeout() && !neglps->satisfiable(context)) {
            return retval_t(BooleanCondition::TRUE_CONSTANT);
        }
    } catch (std::bad_alloc &e) {
        // we are out of memory, deal with it.
        std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
    }

    // Lets try to see if the r1 AND r2 can ever be false at the same time
    // If not, then we know that r1 || r2 must be true.
    // we check this by checking if !r1 && !r2 is unsat

    return retval_t(make_or(conditions), std::make_shared<UnionCollection>(std::move(lps)),
                  std::move(neglps));
}

auto AndCondition::simplify(SimplificationContext &context) const -> retval_t {
    if (context.timeout()) {
        if (context.negated())
            return retval_t(std::make_shared<NotCondition>(make_and(_conds)));
        else
            return retval_t(make_and(_conds));
    }

    if (context.negated())
        return simplifyOr(context);
    else
        return simplify_and(context);
}

auto OrCondition::simplify(SimplificationContext &context) const -> retval_t {
    if (context.timeout()) {
        if (context.negated())
            return retval_t(std::make_shared<NotCondition>(make_or(_conds)));
        else
            return retval_t(make_or(_conds));
    }
    if (context.negated())
        return simplify_and(context);
    else
        return simplifyOr(context);
}

auto CompareConjunction::simplify(SimplificationContext &context) const -> retval_t {
    if (context.timeout()) {
        return retval_t(std::make_shared<CompareConjunction>(*this, context.negated()));
    }
    std::vector<AbstractProgramCollection_ptr> neglps, lpsv;
    auto neg = context.negated() != _negated;
    std::vector<cons_t> nconstraints;
    for (auto &c : _constraints) {
        nconstraints.push_back(c);
        if (c._lower != 0 /*&& !context.timeout()*/) {
            auto m2 = member_for_place(c._place, context);
            Member m1(c._lower);
            // test for trivial comparison
            trivial_e eval = m1 <= m2;
            if (eval != trivial_e::INDETERMINATE) {
                if (eval == trivial_e::FALSE)
                    return retval_t(BooleanCondition::getShared(neg));
                else
                    nconstraints.back()._lower = 0;
            } else { // if no trivial case
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                auto lp =
                    std::make_shared<SingleProgram>(std::move(m1), constant, Simplification::OP_LE);
                auto nlp =
                    std::make_shared<SingleProgram>(std::move(m2), constant, Simplification::OP_GT);
                lpsv.push_back(lp);
                neglps.push_back(nlp);
            }
        }

        if (c._upper != std::numeric_limits<uint32_t>::max() /*&& !context.timeout()*/) {
            auto m1 = member_for_place(c._place, context);
            Member m2(c._upper);
            // test for trivial comparison
            trivial_e eval = m1 <= m2;
            if (eval != trivial_e::INDETERMINATE) {
                if (eval == trivial_e::FALSE)
                    return retval_t(BooleanCondition::getShared(neg));
                else
                    nconstraints.back()._upper = std::numeric_limits<uint32_t>::max();
            } else { // if no trivial case
                int constant = m2.constant() - m1.constant();
                m1 -= m2;
                m2 = m1;
                auto lp =
                    std::make_shared<SingleProgram>(std::move(m1), constant, Simplification::OP_LE);
                auto nlp =
                    std::make_shared<SingleProgram>(std::move(m2), constant, Simplification::OP_GT);
                lpsv.push_back(lp);
                neglps.push_back(nlp);
            }
        }

        assert(nconstraints.size() > 0);
        if (nconstraints.back()._lower == 0 &&
            nconstraints.back()._upper == std::numeric_limits<uint32_t>::max())
            nconstraints.pop_back();

        assert(nconstraints.size() <= neglps.size() * 2);
    }

    auto lps = merge_lps(std::move(lpsv));

    if (lps == nullptr && !context.timeout()) {
        return retval_t(BooleanCondition::getShared(!neg));
    }

    try {
        if (!context.timeout() && lps && !lps->satisfiable(context)) {
            return retval_t(BooleanCondition::getShared(neg));
        }
    } catch (std::bad_alloc &e) {
        // we are out of memory, deal with it.
        std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
    }
    // Lets try to see if the r1 AND r2 can ever be false at the same time
    // If not, then we know that r1 || r2 must be true.
    // we check this by checking if !r1 && !r2 is unsat
    try {
        // remove trivial rules from neglp
        int ncnt = (int)neglps.size() - 1;
        for (int i = ((int)nconstraints.size()) - 1; i >= 0; --i) {
            if (context.timeout())
                break;
            assert(ncnt >= 0);
            size_t cnt = 0;
            auto &c = nconstraints[i];
            if (c._lower != 0)
                ++cnt;
            if (c._upper != std::numeric_limits<uint32_t>::max())
                ++cnt;
            for (size_t j = 0; j < cnt; ++j) {
                assert(ncnt >= 0);
                if (!neglps[ncnt]->satisfiable(context)) {
                    if (j == 1 || c._upper == std::numeric_limits<uint32_t>::max())
                        c._lower = 0;
                    else if (j == 0)
                        c._upper = std::numeric_limits<uint32_t>::max();
                    neglps.erase(neglps.begin() + ncnt);
                }
                if (c._upper == std::numeric_limits<uint32_t>::max() && c._lower == 0)
                    nconstraints.erase(nconstraints.begin() + i);
                --ncnt;
            }
        }
    } catch (std::bad_alloc &e) {
        // we are out of memory, deal with it.
        std::cout << "Query reduction: memory exceeded during LPS merge." << std::endl;
    }
    if (nconstraints.size() == 0) {
        return retval_t(BooleanCondition::getShared(!neg));
    }

    Condition_ptr rc = [&]() -> Condition_ptr {
        if (nconstraints.size() == 1) {
            auto &c = nconstraints[0];
            auto id = std::make_shared<UnfoldedIdentifierExpr>(c._name, c._place);
            auto ll = std::make_shared<LiteralExpr>(c._lower);
            auto lu = std::make_shared<LiteralExpr>(c._upper);
            if (c._lower == c._upper) {
                if (c._lower != 0)
                    if (neg)
                        return std::make_shared<NotEqualCondition>(id, lu);
                    else
                        return std::make_shared<EqualCondition>(id, lu);
                else if (neg)
                    return std::make_shared<LessThanCondition>(lu, id);
                else
                    return std::make_shared<LessThanOrEqualCondition>(id, lu);
            } else {
                if (c._lower != 0 && c._upper != std::numeric_limits<uint32_t>::max()) {
                    if (neg)
                        return make_or(std::make_shared<LessThanCondition>(id, ll),
                                       std::make_shared<LessThanCondition>(lu, id));
                    else
                        return make_and(std::make_shared<LessThanOrEqualCondition>(ll, id),
                                        std::make_shared<LessThanOrEqualCondition>(id, lu));
                } else if (c._lower != 0) {
                    if (neg)
                        return std::make_shared<LessThanCondition>(id, ll);
                    else
                        return std::make_shared<LessThanOrEqualCondition>(ll, id);
                } else {
                    if (neg)
                        return std::make_shared<LessThanCondition>(lu, id);
                    else
                        return std::make_shared<LessThanOrEqualCondition>(id, lu);
                }
            }
        } else {
            return std::make_shared<CompareConjunction>(std::move(nconstraints),
                                                        context.negated() != _negated);
        }
    }();

    if (!neg) {
        return retval_t(rc, std::move(lps), std::make_shared<UnionCollection>(std::move(neglps)));
    } else {
        return retval_t(rc, std::make_shared<UnionCollection>(std::move(neglps)), std::move(lps));
    }
}

auto EqualCondition::simplify(SimplificationContext &context) const -> retval_t {

    Member m1 = _expr1->constraint(context);
    Member m2 = _expr2->constraint(context);
    std::shared_ptr<AbstractProgramCollection> lps, neglps;
    if (!context.timeout() && m1.can_analyze() && m2.can_analyze()) {
        if ((m1.is_zero() && m2.is_zero()) || m1.substration_is_zero(m2)) {
            return retval_t(BooleanCondition::getShared(context.negated()
                                                          ? (m1.constant() != m2.constant())
                                                          : (m1.constant() == m2.constant())));
        } else {
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            m2 = m1;
            Member m3 = m2;
            neglps = std::make_shared<UnionCollection>(
                std::make_shared<SingleProgram>(std::move(m1), constant, Simplification::OP_GT),
                std::make_shared<SingleProgram>(std::move(m2), constant, Simplification::OP_LT));
            lps = std::make_shared<SingleProgram>(std::move(m3), constant, Simplification::OP_EQ);

            if (context.negated())
                lps.swap(neglps);
        }
    } else {
        lps = std::make_shared<SingleProgram>();
        neglps = std::make_shared<SingleProgram>();
    }

    if (!context.timeout() && !lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else if (!context.timeout() && !neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else {
        if (context.negated()) {
            return retval_t(std::make_shared<NotEqualCondition>(_expr1, _expr2), std::move(lps),
                          std::move(neglps));
        } else {
            return retval_t(std::make_shared<EqualCondition>(_expr1, _expr2), std::move(lps),
                          std::move(neglps));
        }
    }
}

auto NotEqualCondition::simplify(SimplificationContext &context) const -> retval_t {
    Member m1 = _expr1->constraint(context);
    Member m2 = _expr2->constraint(context);
    std::shared_ptr<AbstractProgramCollection> lps, neglps;
    if (!context.timeout() && m1.can_analyze() && m2.can_analyze()) {
        if ((m1.is_zero() && m2.is_zero()) || m1.substration_is_zero(m2)) {
            return retval_t(std::make_shared<BooleanCondition>(
                context.negated() ? (m1.constant() == m2.constant())
                                  : (m1.constant() != m2.constant())));
        } else {
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            m2 = m1;
            Member m3 = m2;
            lps = std::make_shared<UnionCollection>(
                std::make_shared<SingleProgram>(std::move(m1), constant, Simplification::OP_GT),
                std::make_shared<SingleProgram>(std::move(m2), constant, Simplification::OP_LT));
            neglps =
                std::make_shared<SingleProgram>(std::move(m3), constant, Simplification::OP_EQ);

            if (context.negated())
                lps.swap(neglps);
        }
    } else {
        lps = std::make_shared<SingleProgram>();
        neglps = std::make_shared<SingleProgram>();
    }
    if (!context.timeout() && !lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else if (!context.timeout() && !neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else {
        if (context.negated()) {
            return retval_t(std::make_shared<EqualCondition>(_expr1, _expr2), std::move(lps),
                          std::move(neglps));
        } else {
            return retval_t(std::make_shared<NotEqualCondition>(_expr1, _expr2), std::move(lps),
                          std::move(neglps));
        }
    }
}

auto LessThanCondition::simplify(SimplificationContext &context) const -> retval_t {
    Member m1 = _expr1->constraint(context);
    Member m2 = _expr2->constraint(context);
    AbstractProgramCollection_ptr lps, neglps;
    if (!context.timeout() && m1.can_analyze() && m2.can_analyze()) {
        // test for trivial comparison
        trivial_e eval = context.negated() ? m1 >= m2 : m1 < m2;
        if (eval != trivial_e::INDETERMINATE) {
            return retval_t(BooleanCondition::getShared(eval == trivial_e::TRUE));
        } else { // if no trivial case
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            m2 = m1;
            lps = std::make_shared<SingleProgram>(
                std::move(m1), constant,
                (context.negated() ? Simplification::OP_GE : Simplification::OP_LT));
            neglps = std::make_shared<SingleProgram>(
                std::move(m2), constant,
                (!context.negated() ? Simplification::OP_GE : Simplification::OP_LT));
        }
    } else {
        lps = std::make_shared<SingleProgram>();
        neglps = std::make_shared<SingleProgram>();
    }

    if (!context.timeout() && !lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else if (!context.timeout() && !neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else {
        if (context.negated()) {
            return retval_t(std::make_shared<LessThanOrEqualCondition>(_expr2, _expr1),
                          std::move(lps), std::move(neglps));
        } else {
            return retval_t(std::make_shared<LessThanCondition>(_expr1, _expr2), std::move(lps),
                          std::move(neglps));
        }
    }
}

auto LessThanOrEqualCondition::simplify(SimplificationContext &context) const -> retval_t {
    Member m1 = _expr1->constraint(context);
    Member m2 = _expr2->constraint(context);

    AbstractProgramCollection_ptr lps, neglps;
    if (!context.timeout() && m1.can_analyze() && m2.can_analyze()) {
        // test for trivial comparison
        trivial_e eval = context.negated() ? m1 > m2 : m1 <= m2;
        if (eval != trivial_e::INDETERMINATE) {
            return retval_t(BooleanCondition::getShared(eval == trivial_e::TRUE));
        } else { // if no trivial case
            int constant = m2.constant() - m1.constant();
            m1 -= m2;
            m2 = m1;
            lps = std::make_shared<SingleProgram>(
                std::move(m1), constant,
                (context.negated() ? Simplification::OP_GT : Simplification::OP_LE));
            neglps = std::make_shared<SingleProgram>(
                std::move(m2), constant,
                (context.negated() ? Simplification::OP_LE : Simplification::OP_GT));
        }
    } else {
        lps = std::make_shared<SingleProgram>();
        neglps = std::make_shared<SingleProgram>();
    }

    assert(lps);
    assert(neglps);

    if (!context.timeout() && !neglps->satisfiable(context)) {
        return retval_t(BooleanCondition::TRUE_CONSTANT);
    } else if (!context.timeout() && !lps->satisfiable(context)) {
        return retval_t(BooleanCondition::FALSE_CONSTANT);
    } else {
        if (context.negated()) {
            return retval_t(std::make_shared<LessThanCondition>(_expr2, _expr1), std::move(lps),
                          std::move(neglps));
        } else {
            return retval_t(std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2),
                          std::move(lps), std::move(neglps));
        }
    }
}

auto NotCondition::simplify(SimplificationContext &context) const -> retval_t {
    context.negate();
    retval_t r = _cond->simplify(context);
    context.negate();
    return r;
}

auto BooleanCondition::simplify(SimplificationContext &context) const -> retval_t {
    if (context.negated()) {
        return retval_t(getShared(!value));
    } else {
        return retval_t(getShared(value));
    }
}

auto DeadlockCondition::simplify(SimplificationContext &context) const -> retval_t {
    if (context.negated()) {
        return retval_t(std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK));
    } else {
        return retval_t(DeadlockCondition::DEADLOCK);
    }
}

auto UnfoldedUpperBoundsCondition::simplify(SimplificationContext &context) const -> retval_t {
    std::vector<place_t> next;
    std::vector<uint32_t> places;
    for (auto &p : _places)
        places.push_back(p._place);
    const auto nplaces = _places.size();
    const auto bounds = LinearProgram::bounds(context, context.get_lp_timeout(), places);
    double offset = _offset;
    for (size_t i = 0; i < nplaces; ++i) {
        if (bounds[i].first != 0 && !bounds[i].second)
            next.emplace_back(_places[i], bounds[i].first);
        if (bounds[i].second)
            offset += bounds[i].first;
    }
    if (bounds[nplaces].second) {
        next.clear();
        return retval_t(std::make_shared<UnfoldedUpperBoundsCondition>(
            next, 0, bounds[nplaces].first + _offset));
    }
    return retval_t(std::make_shared<UnfoldedUpperBoundsCondition>(
        next, bounds[nplaces].first - offset, offset));
}

/******************** Check if query is a reachability query ********************/

auto EXCondition::is_reachability(uint32_t depth) const -> bool { return false; }

auto EGCondition::is_reachability(uint32_t depth) const -> bool { return false; }

auto EFCondition::is_reachability(uint32_t depth) const -> bool {
    return depth > 0 ? false : _cond->is_reachability(depth + 1);
}

auto AXCondition::is_reachability(uint32_t depth) const -> bool { return false; }

auto AGCondition::is_reachability(uint32_t depth) const -> bool {
    return depth > 0 ? false : _cond->is_reachability(depth + 1);
}

auto AFCondition::is_reachability(uint32_t depth) const -> bool { return false; }

auto ECondition::is_reachability(uint32_t depth) const -> bool {
    if (depth != 0) {
        return false;
    }

    if (auto cond = dynamic_cast<FCondition *>(_cond.get())) {
        // EF is a reachability formula so skip checking the F.
        return (*cond)[0]->is_reachability(depth + 1);
    }
    return _cond->is_reachability(depth + 1);
}

auto ACondition::is_reachability(uint32_t depth) const -> bool {
    if (depth != 0) {
        return false;
    }
    if (auto cond = dynamic_cast<GCondition *>(_cond.get())) {
        return (*cond)[0]->is_reachability(depth + 1);
    }
    return _cond->is_reachability(depth + 1);
}

auto UntilCondition::is_reachability(uint32_t depth) const -> bool { return false; }

auto LogicalCondition::is_reachability(uint32_t depth) const -> bool {
    if (depth == 0)
        return false;
    bool reachability = true;
    for (auto &c : _conds) {
        reachability = reachability && c->is_reachability(depth + 1);
        if (!reachability)
            break;
    }
    return reachability;
}

auto CompareCondition::is_reachability(uint32_t depth) const -> bool { return depth > 0; }

auto NotCondition::is_reachability(uint32_t depth) const -> bool {
    return _cond->is_reachability(depth);
}

auto BooleanCondition::is_reachability(uint32_t depth) const -> bool { return depth > 0; }

auto DeadlockCondition::is_reachability(uint32_t depth) const -> bool { return depth > 0; }

auto UnfoldedUpperBoundsCondition::is_reachability(uint32_t depth) const -> bool {
    return depth > 0;
}

/******************** Prepare Reachability Queries ********************/

auto EXCondition::prepare_for_reachability(bool negated) const -> Condition_ptr { return nullptr; }

auto EGCondition::prepare_for_reachability(bool negated) const -> Condition_ptr { return nullptr; }

auto EFCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    _cond->set_invariant(negated);
    return _cond;
}

auto AXCondition::prepare_for_reachability(bool negated) const -> Condition_ptr { return nullptr; }

auto AGCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    Condition_ptr cond = std::make_shared<NotCondition>(_cond);
    cond->set_invariant(!negated);
    return cond;
}

auto AFCondition::prepare_for_reachability(bool negated) const -> Condition_ptr { return nullptr; }

auto ACondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    auto g = std::dynamic_pointer_cast<GCondition>(_cond);
    return g ? AGCondition((*g)[0]).prepare_for_reachability(negated) : nullptr;
}

auto ECondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    auto f = std::dynamic_pointer_cast<FCondition>(_cond);
    return f ? EFCondition((*f)[0]).prepare_for_reachability(negated) : nullptr;
}

auto UntilCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto LogicalCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto CompareConjunction::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto CompareCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto NotCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return _cond->prepare_for_reachability(!negated);
}

auto BooleanCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto DeadlockCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

auto UnfoldedUpperBoundsCondition::prepare_for_reachability(bool negated) const -> Condition_ptr {
    return nullptr;
}

/******************** Prepare CTL Queries ********************/

auto EGCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    ++stats[0];
    return AFCondition(std::make_shared<NotCondition>(_cond))
        .push_negation(stats, context, nested, !negated, initrw);
}

auto AGCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    ++stats[1];
    return EFCondition(std::make_shared<NotCondition>(_cond))
        .push_negation(stats, context, nested, !negated, initrw);
}

auto EXCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto a = _cond->push_negation(stats, context, true, negated, initrw);
            if (negated) {
                ++stats[2];
                return AXCondition(a).push_negation(stats, context, nested, false, initrw);
            } else {
                if (a == BooleanCondition::FALSE_CONSTANT) {
                    ++stats[3];
                    return a;
                }
                if (a == BooleanCondition::TRUE_CONSTANT) {
                    ++stats[4];
                    return std::make_shared<NotCondition>(DeadlockCondition::DEADLOCK);
                }
                a = std::make_shared<EXCondition>(a);
            }
            return a;
        },
        stats, context, nested, negated, initrw);
}

auto AXCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto a = _cond->push_negation(stats, context, true, negated, initrw);
            if (negated) {
                ++stats[5];
                return EXCondition(a).push_negation(stats, context, nested, false, initrw);
            } else {
                if (a == BooleanCondition::TRUE_CONSTANT) {
                    ++stats[6];
                    return a;
                }
                if (a == BooleanCondition::FALSE_CONSTANT) {
                    ++stats[7];
                    return DeadlockCondition::DEADLOCK;
                }
                a = std::make_shared<AXCondition>(a);
            }
            return a;
        },
        stats, context, nested, negated, initrw);
}

auto EFCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto a = _cond->push_negation(stats, context, true, false, initrw);

            if (auto cond = dynamic_cast<NotCondition *>(a.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[8];
                    return a->push_negation(stats, context, nested, negated, initrw);
                }
            }

            if (!a->is_temporal()) {
                auto res = std::make_shared<EFCondition>(a);
                if (negated)
                    return std::make_shared<NotCondition>(res);
                return res;
            }

            if (dynamic_cast<EFCondition *>(a.get())) {
                ++stats[9];
                if (negated)
                    a = std::make_shared<NotCondition>(a);
                return a;
            } else if (auto cond = dynamic_cast<AFCondition *>(a.get())) {
                ++stats[10];
                a = EFCondition((*cond)[0]).push_negation(stats, context, nested, negated, initrw);
                return a;
            } else if (auto cond = dynamic_cast<EUCondition *>(a.get())) {
                ++stats[11];
                a = EFCondition((*cond)[1]).push_negation(stats, context, nested, negated, initrw);
                return a;
            } else if (auto cond = dynamic_cast<AUCondition *>(a.get())) {
                ++stats[12];
                a = EFCondition((*cond)[1]).push_negation(stats, context, nested, negated, initrw);
                return a;
            } else if (auto cond = dynamic_cast<OrCondition *>(a.get())) {
                if (!cond->is_temporal()) {
                    Condition_ptr b = std::make_shared<EFCondition>(a);
                    if (negated)
                        b = std::make_shared<NotCondition>(b);
                    return b;
                }
                ++stats[13];
                std::vector<Condition_ptr> pef, atomic;
                for (auto &i : *cond) {
                    pef.push_back(std::make_shared<EFCondition>(i));
                }
                a = make_or(pef)->push_negation(stats, context, nested, negated, initrw);
                return a;
            } else {
                Condition_ptr b = std::make_shared<EFCondition>(a);
                if (negated)
                    b = std::make_shared<NotCondition>(b);
                return b;
            }
        },
        stats, context, nested, negated, initrw);
}

auto AFCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto a = _cond->push_negation(stats, context, true, false, initrw);
            if (auto cond = dynamic_cast<NotCondition *>(a.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[14];
                    return a->push_negation(stats, context, nested, negated, initrw);
                }
            }

            if (dynamic_cast<AFCondition *>(a.get())) {
                ++stats[15];
                if (negated)
                    return std::make_shared<NotCondition>(a);
                return a;

            } else if (dynamic_cast<EFCondition *>(a.get())) {
                ++stats[16];
                if (negated)
                    return std::make_shared<NotCondition>(a);
                return a;
            } else if (auto cond = dynamic_cast<OrCondition *>(a.get())) {

                std::vector<Condition_ptr> pef, npef;
                for (auto &i : *cond) {
                    if (dynamic_cast<EFCondition *>(i.get())) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[17] += pef.size();
                    pef.push_back(std::make_shared<AFCondition>(make_or(npef)));
                    return make_or(pef)->push_negation(stats, context, nested, negated, initrw);
                }
            } else if (auto cond = dynamic_cast<AUCondition *>(a.get())) {
                ++stats[18];
                return AFCondition((*cond)[1])
                    .push_negation(stats, context, nested, negated, initrw);
            }
            auto b = std::make_shared<AFCondition>(a);
            if (negated)
                return std::make_shared<NotCondition>(b);
            return b;
        },
        stats, context, nested, negated, initrw);
}

auto AUCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto b = _cond2->push_negation(stats, context, true, false, initrw);
            auto a = _cond1->push_negation(stats, context, true, false, initrw);
            if (auto cond = dynamic_cast<NotCondition *>(b.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[19];
                    return b->push_negation(stats, context, nested, negated, initrw);
                }
            } else if (a == DeadlockCondition::DEADLOCK) {
                ++stats[20];
                return b->push_negation(stats, context, nested, negated, initrw);
            } else if (auto cond = dynamic_cast<NotCondition *>(a.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[21];
                    return AFCondition(b).push_negation(stats, context, nested, negated, initrw);
                }
            }

            if (auto cond = dynamic_cast<AFCondition *>(b.get())) {
                ++stats[22];
                return cond->push_negation(stats, context, nested, negated, initrw);
            } else if (dynamic_cast<EFCondition *>(b.get())) {
                ++stats[23];
                if (negated)
                    return std::make_shared<NotCondition>(b);
                return b;
            } else if (auto cond = dynamic_cast<OrCondition *>(b.get())) {
                std::vector<Condition_ptr> pef, npef;
                for (auto &i : *cond) {
                    if (dynamic_cast<EFCondition *>(i.get())) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[24] += pef.size();
                    if (npef.size() != 0) {
                        pef.push_back(std::make_shared<AUCondition>(_cond1, make_or(npef)));
                    } else {
                        ++stats[23];
                        --stats[24];
                    }
                    return make_or(pef)->push_negation(stats, context, nested, negated, initrw);
                }
            }

            auto c = std::make_shared<AUCondition>(a, b);
            if (negated)
                return std::make_shared<NotCondition>(c);
            return c;
        },
        stats, context, nested, negated, initrw);
}

auto EUCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto b = _cond2->push_negation(stats, context, true, false, initrw);
            auto a = _cond1->push_negation(stats, context, true, false, initrw);

            if (auto cond = dynamic_cast<NotCondition *>(b.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[25];
                    return b->push_negation(stats, context, nested, negated, initrw);
                }
            } else if (a == DeadlockCondition::DEADLOCK) {
                ++stats[26];
                return b->push_negation(stats, context, nested, negated, initrw);
            } else if (auto cond = dynamic_cast<NotCondition *>(a.get())) {
                if ((*cond)[0] == DeadlockCondition::DEADLOCK) {
                    ++stats[27];
                    return EFCondition(b).push_negation(stats, context, nested, negated, initrw);
                }
            }

            if (dynamic_cast<EFCondition *>(b.get())) {
                ++stats[28];
                if (negated)
                    return std::make_shared<NotCondition>(b);
                return b;
            } else if (auto cond = dynamic_cast<OrCondition *>(b.get())) {
                std::vector<Condition_ptr> pef, npef;
                for (auto &i : *cond) {
                    if (dynamic_cast<EFCondition *>(i.get())) {
                        pef.push_back(i);
                    } else {
                        npef.push_back(i);
                    }
                }
                if (pef.size() > 0) {
                    stats[29] += pef.size();
                    if (npef.size() != 0) {
                        pef.push_back(std::make_shared<EUCondition>(_cond1, make_or(npef)));
                        ++stats[28];
                        --stats[29];
                    }
                    return make_or(pef)->push_negation(stats, context, nested, negated, initrw);
                }
            }
            auto c = std::make_shared<EUCondition>(a, b);
            if (negated)
                return std::make_shared<NotCondition>(c);
            return c;
        },
        stats, context, nested, negated, initrw);
}

/*LTL negation push*/
auto UntilCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                   bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto b = _cond2->push_negation(stats, context, true, false, initrw);
            auto a = _cond1->push_negation(stats, context, true, false, initrw);

            if (auto cond = std::dynamic_pointer_cast<FCondition>(b)) {
                static_assert(negstat_t::nrules >= 35);
                ++stats[34];
                if (negated)
                    return std::make_shared<NotCondition>(b);
                return b;
            }

            auto c = std::make_shared<UntilCondition>(a, b);
            if (negated)
                return std::make_shared<NotCondition>(c);
            return c;
        },
        stats, context, nested, negated, initrw);
}

auto XCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                               bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto res = _cond->push_negation(stats, context, true, negated, initrw);
            if (res == BooleanCondition::TRUE_CONSTANT || res == BooleanCondition::FALSE_CONSTANT) {
                return res;
            }
            return std::make_shared<XCondition>(res);
        },
        stats, context, nested, negated, initrw);
}

auto FCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                               bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            auto a = _cond->push_negation(stats, context, true, false, initrw);
            if (!a->is_temporal()) {
                auto res = std::make_shared<FCondition>(a);
                if (negated)
                    return std::make_shared<NotCondition>(res);
                return res;
            }

            if (dynamic_cast<FCondition *>(a.get())) {
                ++stats[31];
                if (negated)
                    a = std::make_shared<NotCondition>(a);
                return a;
            } else if (auto cond = dynamic_cast<UntilCondition *>(a.get())) {
                ++stats[32];
                return FCondition((*cond)[1])
                    .push_negation(stats, context, nested, negated, initrw);
            } else if (auto cond = dynamic_cast<OrCondition *>(a.get())) {
                if (!cond->is_temporal()) {
                    Condition_ptr b = std::make_shared<FCondition>(a);
                    if (negated)
                        b = std::make_shared<NotCondition>(b);
                    return b;
                }
                ++stats[33];
                std::vector<Condition_ptr> distributed;
                for (auto &i : *cond) {
                    distributed.push_back(std::make_shared<FCondition>(i));
                }
                return make_or(distributed)->push_negation(stats, context, nested, negated, initrw);
            } else {
                Condition_ptr b = std::make_shared<FCondition>(a);
                if (negated)
                    b = std::make_shared<NotCondition>(b);
                return b;
            }
        },
        stats, context, nested, negated, initrw);
}

auto ACondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                               bool negated, bool initrw) -> Condition_ptr {
    return ECondition(std::make_shared<NotCondition>(_cond))
        .push_negation(stats, context, nested, !negated, initrw);
}

auto ECondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                               bool negated, bool initrw) -> Condition_ptr {
    // we forward the negated flag, we flip the outer quantifier later!
    auto _sub = _cond->push_negation(stats, context, nested, negated, initrw);
    if (negated)
        return std::make_shared<ACondition>(_sub);
    else
        return std::make_shared<ECondition>(_sub);
}

auto GCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                               bool negated, bool initrw) -> Condition_ptr {
    return FCondition(std::make_shared<NotCondition>(_cond))
        .push_negation(stats, context, nested, !negated, initrw);
}

/*Boolean connectives */
auto push_and(const std::vector<Condition_ptr> &_conds, negstat_t &stats,
              const EvaluationContext &context, bool nested, bool negate_children, bool initrw)
    -> Condition_ptr {
    std::vector<Condition_ptr> nef, other;
    for (auto &c : _conds) {
        auto n = c->push_negation(stats, context, nested, negate_children, initrw);
        if (n->is_trivially_false())
            return n;
        if (n->is_trivially_true())
            continue;
        if (auto neg = dynamic_cast<NotCondition *>(n.get())) {
            if (auto ef = dynamic_cast<EFCondition *>((*neg)[0].get())) {
                nef.push_back((*ef)[0]);
            } else {
                other.emplace_back(n);
            }
        } else {
            other.emplace_back(n);
        }
    }
    if (nef.size() + other.size() == 0)
        return BooleanCondition::TRUE_CONSTANT;
    if (nef.size() + other.size() == 1) {
        return nef.size() == 0
                   ? other[0]
                   : std::make_shared<NotCondition>(std::make_shared<EFCondition>(nef[0]));
    }
    if (nef.size() != 0)
        other.push_back(
            std::make_shared<NotCondition>(std::make_shared<EFCondition>(make_or(nef))));
    if (other.size() == 1)
        return other[0];
    auto res = make_and(other);
    return res;
}

auto push_or(const std::vector<Condition_ptr> &_conds, negstat_t &stats,
             const EvaluationContext &context, bool nested, bool negate_children, bool initrw)
    -> Condition_ptr {
    std::vector<Condition_ptr> nef, other;
    for (auto &c : _conds) {
        auto n = c->push_negation(stats, context, nested, negate_children, initrw);
        if (n->is_trivially_true()) {
            return n;
        }
        if (n->is_trivially_false())
            continue;
        if (auto ef = dynamic_cast<EFCondition *>(n.get())) {
            nef.push_back((*ef)[0]);
        } else {
            other.emplace_back(n);
        }
    }
    if (nef.size() + other.size() == 0)
        return BooleanCondition::FALSE_CONSTANT;
    if (nef.size() + other.size() == 1) {
        return nef.size() == 0 ? other[0] : std::make_shared<EFCondition>(nef[0]);
    }
    if (nef.size() != 0)
        other.push_back(std::make_shared<EFCondition>(make_or(nef)));
    if (other.size() == 1)
        return other[0];
    return make_or(other);
}

auto OrCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            return negated ? push_and(_conds, stats, context, nested, true, initrw)
                           : push_or(_conds, stats, context, nested, false, initrw);
        },
        stats, context, nested, negated, initrw);
}

auto AndCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                 bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            return negated ? push_or(_conds, stats, context, nested, true, initrw)
                           : push_and(_conds, stats, context, nested, false, initrw);
        },
        stats, context, nested, negated, initrw);
}

auto CompareConjunction::push_negation(negstat_t &stats, const EvaluationContext &context,
                                       bool nested, bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr { return std::make_shared<CompareConjunction>(*this, negated); },
        stats, context, nested, negated, initrw);
}

auto NotCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                 bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            if (negated)
                ++stats[30];
            return _cond->push_negation(stats, context, nested, !negated, initrw);
        },
        stats, context, nested, negated, initrw);
}

template <typename T>
auto push_fireable_negation(negstat_t &stat, const EvaluationContext &context, bool nested,
                            bool negated, bool initrw, const std::string &name,
                            const Condition_ptr &compiled) -> Condition_ptr {
    if (compiled)
        return compiled->push_negation(stat, context, nested, negated, initrw);
    if (negated) {
        stat._negated_fireability = true;
        return std::make_shared<NotCondition>(std::make_shared<T>(name));
    } else
        return std::make_shared<T>(name);
}

auto UnfoldedFireableCondition::push_negation(negstat_t &stat, const EvaluationContext &context,
                                              bool nested, bool negated, bool initrw)
    -> Condition_ptr {
    return push_fireable_negation<UnfoldedFireableCondition>(stat, context, nested, negated, initrw,
                                                             _name, _compiled);
}

auto FireableCondition::push_negation(negstat_t &stat, const EvaluationContext &context,
                                      bool nested, bool negated, bool initrw) -> Condition_ptr {
    return push_fireable_negation<FireableCondition>(stat, context, nested, negated, initrw, _name,
                                                     _compiled);
}

auto CompareCondition::is_trivial() const -> bool {
    auto remdup = [](auto &a, auto &b) {
        auto ai = a->_ids.begin();
        auto bi = b->_ids.begin();
        while (ai != a->_ids.end() && bi != b->_ids.end()) {
            while (ai != a->_ids.end() && ai->first < bi->first)
                ++ai;
            if (ai == a->_ids.end())
                break;
            if (ai->first == bi->first) {
                ai = a->_ids.erase(ai);
                bi = b->_ids.erase(bi);
            } else {
                ++bi;
            }
            if (bi == b->_ids.end() || ai == a->_ids.end())
                break;
        }
    };
    if (auto p1 = std::dynamic_pointer_cast<PlusExpr>(_expr1))
        if (auto p2 = std::dynamic_pointer_cast<PlusExpr>(_expr2))
            remdup(p1, p2);

    if (auto m1 = std::dynamic_pointer_cast<MultiplyExpr>(_expr1))
        if (auto m2 = std::dynamic_pointer_cast<MultiplyExpr>(_expr2))
            remdup(m1, m2);

    if (auto p1 = std::dynamic_pointer_cast<CommutativeExpr>(_expr1))
        if (auto p2 = std::dynamic_pointer_cast<CommutativeExpr>(_expr2))
            return p1->_exprs.size() + p1->_ids.size() + p2->_exprs.size() + p2->_ids.size() == 0;
    return _expr1->place_free() && _expr2->place_free();
}

auto LessThanCondition::push_negation(negstat_t &stats, const EvaluationContext &context,
                                      bool nested, bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            if (is_trivial())
                return BooleanCondition::getShared(evaluate(context) xor negated);
            if (negated)
                return std::make_shared<LessThanOrEqualCondition>(_expr2, _expr1);
            else
                return std::make_shared<LessThanCondition>(_expr1, _expr2);
        },
        stats, context, nested, negated, initrw);
}

auto LessThanOrEqualCondition::push_negation(negstat_t &stats, const EvaluationContext &context,
                                             bool nested, bool negated, bool initrw)
    -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            if (is_trivial())
                return BooleanCondition::getShared(evaluate(context) xor negated);
            if (negated)
                return std::make_shared<LessThanCondition>(_expr2, _expr1);
            else
                return std::make_shared<LessThanOrEqualCondition>(_expr1, _expr2);
        },
        stats, context, nested, negated, initrw);
}

auto push_equal(CompareCondition *org, bool negated, bool noteq, const EvaluationContext &context)
    -> Condition_ptr {
    if (org->is_trivial())
        return BooleanCondition::getShared(org->evaluate(context) xor negated);
    for (auto i : {0, 1}) {
        if ((*org)[i]->place_free() && (*org)[i]->evaluate(context) == 0) {
            if (negated == noteq)
                return std::make_shared<LessThanOrEqualCondition>((*org)[(i + 1) % 2],
                                                                  std::make_shared<LiteralExpr>(0));
            else
                return std::make_shared<LessThanOrEqualCondition>(std::make_shared<LiteralExpr>(1),
                                                                  (*org)[(i + 1) % 2]);
        }
    }
    if (negated == noteq)
        return std::make_shared<EqualCondition>((*org)[0], (*org)[1]);
    else
        return std::make_shared<NotEqualCondition>((*org)[0], (*org)[1]);
}

auto NotEqualCondition::push_negation(negstat_t &stats, const EvaluationContext &context,
                                      bool nested, bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr { return push_equal(this, negated, true, context); }, stats, context,
        nested, negated, initrw);
}

auto EqualCondition::push_negation(negstat_t &stats, const EvaluationContext &context, bool nested,
                                   bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr { return push_equal(this, negated, false, context); }, stats,
        context, nested, negated, initrw);
}

auto BooleanCondition::push_negation(negstat_t &stats, const EvaluationContext &context,
                                     bool nested, bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            if (negated)
                return getShared(!value);
            else
                return getShared(value);
        },
        stats, context, nested, negated, initrw);
}

auto DeadlockCondition::push_negation(negstat_t &stats, const EvaluationContext &context,
                                      bool nested, bool negated, bool initrw) -> Condition_ptr {
    return initial_marking_rewrite(
        [&]() -> Condition_ptr {
            if (negated)
                return std::make_shared<NotCondition>(DEADLOCK);
            else
                return DEADLOCK;
        },
        stats, context, nested, negated, initrw);
}

auto UnfoldedUpperBoundsCondition::push_negation(negstat_t &, const EvaluationContext &context,
                                                 bool nested, bool negated, bool initrw)
    -> Condition_ptr {
    if (negated) {
        throw base_error_t("UPPER BOUNDS CANNOT BE NEGATED!");
    }
    return std::make_shared<UnfoldedUpperBoundsCondition>(_places, _max, _offset);
}

/********************** CONSTRUCTORS *********************************/

void post_merge(std::vector<Condition_ptr> &conds) {
    std::sort(std::begin(conds), std::end(conds),
              [](auto &a, auto &b) { return a->is_temporal() < b->is_temporal(); });
}

AndCondition::AndCondition(std::vector<Condition_ptr> &&conds) {
    for (auto &c : conds)
        try_merge<AndCondition>(_conds, c);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

AndCondition::AndCondition(const std::vector<Condition_ptr> &conds) {
    for (auto &c : conds)
        try_merge<AndCondition>(_conds, c);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

AndCondition::AndCondition(const Condition_ptr &left, const Condition_ptr &right) {
    try_merge<AndCondition>(_conds, left);
    try_merge<AndCondition>(_conds, right);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

OrCondition::OrCondition(std::vector<Condition_ptr> &&conds) {
    for (auto &c : conds)
        try_merge<OrCondition>(_conds, c);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

OrCondition::OrCondition(const std::vector<Condition_ptr> &conds) {
    for (auto &c : conds)
        try_merge<OrCondition>(_conds, c);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

OrCondition::OrCondition(const Condition_ptr &left, const Condition_ptr &right) {
    try_merge<OrCondition>(_conds, left);
    try_merge<OrCondition>(_conds, right);
    for (auto &c : _conds)
        _temporal = _temporal || c->is_temporal();
    for (auto &c : _conds)
        _loop_sensitive = _loop_sensitive || c->is_loop_sensitive();
    post_merge(_conds);
}

CompareConjunction::CompareConjunction(const std::vector<Condition_ptr> &conditions, bool negated) {
    _negated = negated;
    merge(conditions, negated);
}

void CompareConjunction::merge(const CompareConjunction &other) {
    auto neg = _negated != other._negated;
    if (neg && other._constraints.size() > 1) {
        throw base_error_t("MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED");
    }
    auto il = _constraints.begin();
    for (auto c : other._constraints) {
        if (neg)
            c.invert();

        if (c._upper == std::numeric_limits<uint32_t>::max() && c._lower == 0) {
            continue;
        } else if (c._upper != std::numeric_limits<uint32_t>::max() && c._lower != 0 && neg) {
            throw base_error_t("MERGE OF CONJUNCT AND DISJUNCT NOT ALLOWED");
        }

        il = std::lower_bound(_constraints.begin(), _constraints.end(), c);
        if (il == _constraints.end() || il->_place != c._place) {
            il = _constraints.insert(il, c);
        } else {
            il->_lower = std::max(il->_lower, c._lower);
            il->_upper = std::min(il->_upper, c._upper);
        }
    }
}

void CompareConjunction::merge(const std::vector<Condition_ptr> &conditions, bool negated) {
    for (auto &c : conditions) {
        auto cmp = dynamic_cast<CompareCondition *>(c.get());
        assert(cmp);
        auto id = dynamic_cast<UnfoldedIdentifierExpr *>((*cmp)[0].get());
        uint32_t val;
        bool inverted = false;
        EvaluationContext context;
        if (!id) {
            id = dynamic_cast<UnfoldedIdentifierExpr *>((*cmp)[1].get());
            val = (*cmp)[0]->evaluate(context);
            inverted = true;
        } else {
            val = (*cmp)[1]->evaluate(context);
        }
        assert(id);
        cons_t next;
        next._place = id->offset();

        if (dynamic_cast<LessThanOrEqualCondition *>(c.get()))
            if (inverted)
                next._lower = val;
            else
                next._upper = val;
        else if (dynamic_cast<LessThanCondition *>(c.get()))
            if (inverted)
                next._lower = val + 1;
            else
                next._upper = val - 1;
        else if (dynamic_cast<EqualCondition *>(c.get())) {
            assert(!negated);
            next._lower = val;
            next._upper = val;
        } else if (dynamic_cast<NotEqualCondition *>(c.get())) {
            assert(negated);
            next._lower = val;
            next._upper = val;
            negated = false; // we already handled negation here!
        } else {
            throw base_error_t("Unknown Error in CompareConjunction::merge");
        }
        if (negated)
            next.invert();

        auto lb = std::lower_bound(std::begin(_constraints), std::end(_constraints), next);
        if (lb == std::end(_constraints) || lb->_place != next._place) {
            next._name = id->name();
            _constraints.insert(lb, next);
        } else {
            assert(id->name().compare(lb->_name) == 0);
            lb->intersect(next);
        }
    }
}

void CommutativeExpr::init(std::vector<Expr_ptr> &&exprs) {
    for (auto &e : exprs) {
        if (e->place_free()) {
            EvaluationContext c;
            _constant = apply(_constant, e->evaluate(c));
        } else if (auto id = std::dynamic_pointer_cast<PQL::UnfoldedIdentifierExpr>(e)) {
            _ids.emplace_back(id->offset(), id->name());
        } else if (auto c = std::dynamic_pointer_cast<CommutativeExpr>(e)) {
            // we should move up plus/multiply here when possible;
            if (c->_ids.size() == 0 && c->_exprs.size() == 0) {
                _constant = apply(_constant, c->_constant);
            } else {
                _exprs.emplace_back(std::move(e));
            }
        } else {
            _exprs.emplace_back(std::move(e));
        }
    }
}

PlusExpr::PlusExpr(std::vector<Expr_ptr> &&exprs, bool tk) : CommutativeExpr(0), _tk(tk) {
    init(std::move(exprs));
}

MultiplyExpr::MultiplyExpr(std::vector<Expr_ptr> &&exprs) : CommutativeExpr(1) {
    init(std::move(exprs));
}

auto LogicalCondition::nested_deadlock() const -> bool {
    for (auto &c : _conds) {
        if (c->get_quantifier() == PQL::DEADLOCK || c->nested_deadlock() ||
            (c->get_quantifier() == PQL::NEG &&
             (*static_cast<NotCondition *>(c.get()))[0]->get_quantifier() == PQL::DEADLOCK)) {
            return true;
        }
    }
    return false;
}

} // namespace PetriEngine::PQL
