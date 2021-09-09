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
#ifndef EXPRESSIONS_H
#define EXPRESSIONS_H

#include "..//Simplification/Member.h"
#include "../Simplification/LinearPrograms.h"
#include "../Simplification/retval_t.h"
#include "Contexts.h"
#include "PQL.h"
#include "errorcodes.h"
#include <PetriEngine/Stubborn/StubbornSet.h>
#include <algorithm>
#include <fstream>
#include <iostream>

using namespace PetriEngine::Simplification;

namespace PetriEngine::PQL {

auto generate_tabs(uint32_t tabs) -> std::string;
class CompareCondition;
class NotCondition;
/******************** EXPRESSIONS ********************/

/** Base class for all binary expressions */
class NaryExpr : public Expr {
  protected:
    NaryExpr() = default;

  public:
    NaryExpr(std::vector<Expr_ptr> &&exprs) : _exprs(std::move(exprs)) {}
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override;
    [[nodiscard]] auto formula_size() const -> int override {
        size_t sum = 0;
        for (auto &e : _exprs)
            sum += e->formula_size();
        return sum + 1;
    }
    [[nodiscard]] auto place_free() const -> bool override;
    [[nodiscard]] auto expressions() const -> auto & { return _exprs; }
    [[nodiscard]] auto operands() const -> size_t { return _exprs.size(); }
    auto operator[](size_t i) const -> const Expr_ptr & { return _exprs[i]; }

  protected:
    [[nodiscard]] virtual auto apply(int v1, int v2) const -> int = 0;
    [[nodiscard]] virtual auto op() const -> std::string = 0;
    std::vector<Expr_ptr> _exprs;
    [[nodiscard]] virtual auto pre_op(const EvaluationContext &context) const -> int32_t;
};

class PlusExpr;
class MultiplyExpr;

class CommutativeExpr : public NaryExpr {
  public:
    friend CompareCondition;
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override;
    void to_binary(std::ostream &) const override;
    [[nodiscard]] auto formula_size() const -> int override {
        size_t sum = _ids.size();
        for (auto &e : _exprs)
            sum += e->formula_size();
        return sum + 1;
    }
    [[nodiscard]] auto place_free() const -> bool override;
    [[nodiscard]] auto constant() const { return _constant; }
    [[nodiscard]] auto places() const -> auto & { return _ids; }

  protected:
    CommutativeExpr(int constant) : _constant(constant){};
    void init(std::vector<Expr_ptr> &&exprs);
    [[nodiscard]] auto pre_op(const EvaluationContext &context) const -> int32_t override;
    int32_t _constant;
    std::vector<std::pair<uint32_t, std::string>> _ids;
    auto commutative_cons(int constant, SimplificationContext &context,
                          const std::function<void(Member &a, Member b)> &op) const -> Member;
};

/** Binary plus expression */
class PlusExpr : public CommutativeExpr {
  public:
    PlusExpr(std::vector<Expr_ptr> &&exprs, bool tk = false);

    [[nodiscard]] auto type() const -> Expr::types_e override;
    auto constraint(SimplificationContext &context) const -> Member override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;
    bool _tk = false;

    void visit(Visitor &visitor) const override;

  protected:
    [[nodiscard]] auto apply(int v1, int v2) const -> int override;
    // int binaryOp() const;
    [[nodiscard]] auto op() const -> std::string override;
};

/** Binary minus expression */
class SubtractExpr : public NaryExpr {
  public:
    SubtractExpr(std::vector<Expr_ptr> &&exprs) : NaryExpr(std::move(exprs)) {}
    [[nodiscard]] auto type() const -> Expr::types_e override;
    auto constraint(SimplificationContext &context) const -> Member override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;

    void to_binary(std::ostream &) const override;
    void visit(Visitor &visitor) const override;

  protected:
    [[nodiscard]] auto apply(int v1, int v2) const -> int override;
    // int binaryOp() const;
    [[nodiscard]] auto op() const -> std::string override;
};

/** Binary multiplication expression **/
class MultiplyExpr : public CommutativeExpr {
  public:
    MultiplyExpr(std::vector<Expr_ptr> &&exprs);
    [[nodiscard]] auto type() const -> Expr::types_e override;
    auto constraint(SimplificationContext &context) const -> Member override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;

    void visit(Visitor &visitor) const override;

  protected:
    [[nodiscard]] auto apply(int v1, int v2) const -> int override;
    // int binaryOp() const;
    [[nodiscard]] auto op() const -> std::string override;
};

/** Unary minus expression*/
class MinusExpr : public Expr {
  public:
    MinusExpr(const Expr_ptr &expr) { _expr = expr; }
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override;
    [[nodiscard]] auto type() const -> Expr::types_e override;
    auto constraint(SimplificationContext &context) const -> Member override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;
    void to_binary(std::ostream &) const override;

    void visit(Visitor &visitor) const override;
    [[nodiscard]] auto formula_size() const -> int override { return _expr->formula_size() + 1; }
    [[nodiscard]] auto place_free() const -> bool override;
    auto operator[](size_t i) const -> const Expr_ptr & { return _expr; };

  private:
    Expr_ptr _expr;
};

/** Literal integer value expression */
class LiteralExpr : public Expr {
  public:
    LiteralExpr(int value) : _value(value) {}
    LiteralExpr(const LiteralExpr &) = default;
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override;
    [[nodiscard]] auto type() const -> Expr::types_e override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;
    void to_binary(std::ostream &) const override;

    void visit(Visitor &visitor) const override;
    [[nodiscard]] auto formula_size() const -> int override { return 1; }
    [[nodiscard]] auto value() const -> int { return _value; };
    auto constraint(SimplificationContext &context) const -> Member override;
    [[nodiscard]] auto place_free() const -> bool override { return true; }

  private:
    int _value;
};

class IdentifierExpr : public Expr {
  public:
    IdentifierExpr(const std::string &name) : _name(name) {}
    IdentifierExpr(const IdentifierExpr &) = default;
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override {
        return _compiled->evaluate(context);
    }
    [[nodiscard]] auto type() const -> Expr::types_e override {
        if (_compiled)
            return _compiled->type();
        return Expr::IDENTIFIER_EXPR;
    }
    void to_xml(std::ostream &os, uint32_t tabs, bool tokencount = false) const override {
        _compiled->to_xml(os, tabs, tokencount);
    }

    [[nodiscard]] auto formula_size() const -> int override {
        if (_compiled)
            return _compiled->formula_size();
        return 1;
    }
    [[nodiscard]] auto place_free() const -> bool override {
        if (_compiled)
            return _compiled->place_free();
        return false;
    }

    auto constraint(SimplificationContext &context) const -> Member override {
        return _compiled->constraint(context);
    }

    void to_binary(std::ostream &s) const override { _compiled->to_binary(s); }
    void visit(Visitor &visitor) const override;

    [[nodiscard]] auto name() const -> const std::string & { return _name; }

    [[nodiscard]] auto compiled() const -> const Expr_ptr & { return _compiled; }

  private:
    std::string _name;
    Expr_ptr _compiled;
};

/** Identifier expression */
class UnfoldedIdentifierExpr : public Expr {
  public:
    UnfoldedIdentifierExpr(const std::string &name, int offest)
        : _offsetInMarking(offest), _name(name) {}

    UnfoldedIdentifierExpr(const std::string &name) : UnfoldedIdentifierExpr(name, -1) {}

    UnfoldedIdentifierExpr(const UnfoldedIdentifierExpr &) = default;

    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> int override;
    [[nodiscard]] auto type() const -> Expr::types_e override;
    void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const override;
    void to_binary(std::ostream &) const override;
    [[nodiscard]] auto formula_size() const -> int override { return 1; }
    /** Offset in marking or valuation */
    [[nodiscard]] auto offset() const -> int { return _offsetInMarking; }
    [[nodiscard]] auto name() const -> const std::string & { return _name; }
    auto constraint(SimplificationContext &context) const -> Member override;
    [[nodiscard]] auto place_free() const -> bool override { return false; }
    void visit(Visitor &visitor) const override;

  private:
    /** Offset in marking, -1 if undefined, should be resolved during analysis */
    int _offsetInMarking;
    /** Identifier text */
    std::string _name;
};

class ShallowCondition : public Condition {
    auto evaluate(const EvaluationContext &context) -> result_e override {
        return _compiled->evaluate(context);
    }
    auto eval_and_set(const EvaluationContext &context) -> result_e override {
        return _compiled->eval_and_set(context);
    }
    auto distance(DistanceContext &context) const -> uint32_t override {
        return _compiled->distance(context);
    }
    void to_tapaal_query(std::ostream &out, TAPAALConditionExportContext &context) const override {
        _compiled->to_tapaal_query(out, context);
    }
    void to_binary(std::ostream &out) const override { return _compiled->to_binary(out); }
    auto simplify(SimplificationContext &context) const -> retval_t override {
        return _compiled->simplify(context);
    }
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override {
        return _compiled->prepare_for_reachability(negated);
    }
    auto is_reachability(uint32_t depth) const -> bool override {
        return _compiled->is_reachability(depth);
    }

    void to_xml(std::ostream &out, uint32_t tabs) const override { _compiled->to_xml(out, tabs); }

    auto get_quantifier() const -> quantifier_e override { return _compiled->get_quantifier(); }
    auto get_path() const -> path_e override { return _compiled->get_path(); }
    auto get_query_type() const -> ctl_type_e override { return _compiled->get_query_type(); }
    auto contains_next() const -> bool override { return _compiled->contains_next(); }
    auto nested_deadlock() const -> bool override { return _compiled->nested_deadlock(); }
    auto formula_size() const -> int override { return _compiled->formula_size(); }

    auto push_negation(negstat_t &neg, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override {
        if (_compiled)
            return _compiled->push_negation(neg, context, nested, negated, initrw);
        else {
            if (negated)
                return std::static_pointer_cast<Condition>(std::make_shared<NotCondition>(clone()));
            else
                return clone();
        }
    }

    void analyze(AnalysisContext &context) override {
        if (_compiled)
            _compiled->analyze(context);
        else
            internal_analyze(context);
    }

  public:
    auto get_compiled() const -> const Condition_ptr & { return _compiled; }

  protected:
    virtual void internal_analyze(AnalysisContext &context) = 0;
    virtual auto clone() -> Condition_ptr = 0;
    Condition_ptr _compiled = nullptr;
};

/* Not condition */
class NotCondition : public Condition {
  public:
    NotCondition(const Condition_ptr &cond) {
        _cond = cond;
        _temporal = _cond->is_temporal();
        _loop_sensitive = _cond->is_loop_sensitive();
    }
    auto formula_size() const -> int override { return _cond->formula_size() + 1; }

    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    void to_binary(std::ostream &) const override;

    auto get_quantifier() const -> quantifier_e override { return quantifier_e::NEG; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::LOPERATOR; }
    auto operator[](size_t i) const -> const Condition_ptr & { return _cond; };
    auto get_cond() const -> const Condition_ptr & { return _cond; };
    auto is_temporal() const -> bool override { return _temporal; }
    auto contains_next() const -> bool override { return _cond->contains_next(); }
    auto nested_deadlock() const -> bool override { return _cond->nested_deadlock(); }

  private:
    Condition_ptr _cond;
    bool _temporal = false;
};

/******************** TEMPORAL OPERATORS ********************/

class QuantifierCondition : public Condition {
  public:
    auto is_temporal() const -> bool override { return true; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::PATHQEURY; }
    virtual auto operator[](size_t i) const -> const Condition_ptr & = 0;
};

class SimpleQuantifierCondition : public QuantifierCondition {
  public:
    SimpleQuantifierCondition(const Condition_ptr &cond) {
        _cond = cond;
        _loop_sensitive = cond->is_loop_sensitive();
    }
    auto formula_size() const -> int override { return _cond->formula_size() + 1; }

    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    void to_binary(std::ostream &out) const override;

    auto operator[](size_t i) const -> const Condition_ptr & override { return _cond; }
    auto get_cond() const -> const Condition_ptr & { return _cond; }
    auto contains_next() const -> bool override { return _cond->contains_next(); }
    auto nested_deadlock() const -> bool override { return _cond->nested_deadlock(); }

  private:
    virtual auto op() const -> std::string = 0;

  protected:
    Condition_ptr _cond;
};

class ECondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto evaluate(const EvaluationContext &context) -> result_e override;

    auto simplify(SimplificationContext &context) const -> retval_t override;

    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::E; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto distance(DistanceContext &context) const -> uint32_t override {
        // TODO implement
        throw base_error_t("TODO implement");
    }
    auto is_loop_sensitive() const -> bool override {
        // Other LTL Loop sensitivity depend on the outermost quantifier being an A,
        // so if it is an E we disable loop sensitive reductions.
        return true;
    }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class ACondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto evaluate(const EvaluationContext &context) -> result_e override;

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::A; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto distance(DistanceContext &context) const -> uint32_t override {
        uint32_t retval = _cond->distance(context);
        return retval;
    }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class GCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto evaluate(const EvaluationContext &context) -> result_e override;

    auto is_reachability(uint32_t depth) const -> bool override {
        // This could potentially be a reachability formula if the parent is an A.
        // This case is however already handled by ACondition.
        return false;
    }

    auto prepare_for_reachability(bool negated) const -> Condition_ptr override {
        // TODO implement
        throw base_error_t("TODO implement");
    }

    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;

    auto simplify(SimplificationContext &context) const -> retval_t override;

    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }

    auto get_path() const -> path_e override { return path_e::G; }

    auto distance(DistanceContext &context) const -> uint32_t override {
        context.negate();
        uint32_t retval = _cond->distance(context);
        context.negate();
        return retval;
    }

    auto eval_and_set(const EvaluationContext &context) -> result_e override;

    auto is_loop_sensitive() const -> bool override { return true; }

    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class FCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto evaluate(const EvaluationContext &context) -> result_e override;

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override {
        // This could potentially be a reachability formula if the parent is an E.
        // This case is however already handled by ECondition.
        return false;
    }
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override {
        // TODO implement
        throw base_error_t("TODO implement");
    }
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }
    auto get_path() const -> path_e override { return path_e::F; }
    auto distance(DistanceContext &context) const -> uint32_t override {
        return _cond->distance(context);
    }
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    auto is_loop_sensitive() const -> bool override { return true; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class XCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto is_reachability(uint32_t depth) const -> bool override { return false; }
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override {
        // TODO implement
        throw base_error_t("TODO implement");
    }
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }
    auto get_path() const -> path_e override { return path_e::X; }
    auto distance(DistanceContext &context) const -> uint32_t override {
        return _cond->distance(context);
    }
    auto contains_next() const -> bool override { return true; }
    auto is_loop_sensitive() const -> bool override { return true; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class EXCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::E; }
    auto get_path() const -> path_e override { return path_e::X; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto contains_next() const -> bool override { return true; }
    auto is_loop_sensitive() const -> bool override { return true; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class EGCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::E; }
    auto get_path() const -> path_e override { return path_e::G; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    auto is_loop_sensitive() const -> bool override { return true; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class EFCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::E; }
    auto get_path() const -> path_e override { return path_e::F; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class AXCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::A; }
    auto get_path() const -> path_e override { return path_e::X; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto contains_next() const -> bool override { return true; }
    auto is_loop_sensitive() const -> bool override { return true; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class AGCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::A; }
    auto get_path() const -> path_e override { return path_e::G; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto op() const -> std::string override;
};

class AFCondition : public SimpleQuantifierCondition {
  public:
    using SimpleQuantifierCondition::SimpleQuantifierCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::A; }
    auto get_path() const -> path_e override { return path_e::F; }
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto is_loop_sensitive() const -> bool override { return true; }

  private:
    auto op() const -> std::string override;
};

class UntilCondition : public QuantifierCondition {
  public:
    UntilCondition(const Condition_ptr &cond1, const Condition_ptr &cond2) {
        _cond1 = cond1;
        _cond2 = cond2;
        _loop_sensitive = _cond1->is_loop_sensitive() || _cond2->is_loop_sensitive();
    }
    auto formula_size() const -> int override {
        return _cond1->formula_size() + _cond2->formula_size() + 1;
    }

    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    void to_binary(std::ostream &out) const override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;

    auto eval_and_set(const EvaluationContext &context) -> result_e override;

    [[nodiscard]] auto operator[](size_t i) const -> const Condition_ptr & override {
        if (i == 0)
            return _cond1;
        return _cond2;
    }
    auto get_path() const -> path_e override { return path_e::U; }
    auto contains_next() const -> bool override {
        return _cond1->contains_next() || _cond2->contains_next();
    }
    auto nested_deadlock() const -> bool override {
        return _cond1->nested_deadlock() || _cond2->nested_deadlock();
    }

    [[nodiscard]] auto get_cond1() const -> const Condition_ptr & { return (*this)[0]; }
    [[nodiscard]] auto get_cond2() const -> const Condition_ptr & { return (*this)[1]; }

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto distance(DistanceContext &context) const -> uint32_t override {
        return (*this)[1]->distance(context);
    }
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }

  private:
    virtual auto op() const -> std::string;

  protected:
    Condition_ptr _cond1;
    Condition_ptr _cond2;
};

class EUCondition : public UntilCondition {
  public:
    using UntilCondition::UntilCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::E; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

  private:
    auto op() const -> std::string override;
};

class AUCondition : public UntilCondition {
  public:
    using UntilCondition::UntilCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::A; }
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    auto is_loop_sensitive() const -> bool override { return true; }

  private:
    auto op() const -> std::string override;
};

/******************** CONDITIONS ********************/

class UnfoldedFireableCondition : public ShallowCondition {
  public:
    UnfoldedFireableCondition(const std::string &tname) : ShallowCondition(), _name(tname){};
    auto push_negation(negstat_t &stat, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto get_name() const -> std::string { return _name; }
    void to_xml(std::ostream &out, uint32_t tabs) const override;

  protected:
    void internal_analyze(AnalysisContext &context) override;

    auto clone() -> Condition_ptr override {
        return std::make_shared<UnfoldedFireableCondition>(_name);
    }

  public:
  private:
    const std::string _name;
};

class FireableCondition : public ShallowCondition {
  public:
    FireableCondition(const std::string &tname) : _name(tname){};
    auto push_negation(negstat_t &stat, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto get_name() const -> std::string { return _name; }

  protected:
    void internal_analyze(AnalysisContext &context) override;
    auto clone() -> Condition_ptr override { return std::make_shared<FireableCondition>(_name); }

  private:
    const std::string _name;
};

/* Logical conditon */
class LogicalCondition : public Condition {
  public:
    auto formula_size() const -> int override {
        size_t i = 1;
        for (auto &c : _conds)
            i += c->formula_size();
        return i;
    }

    void analyze(AnalysisContext &context) override;

    void to_binary(std::ostream &out) const override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto operator[](size_t i) const -> const Condition_ptr & { return _conds[i]; };
    auto operands() const -> size_t { return _conds.size(); }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::LOPERATOR; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }

    auto is_temporal() const -> bool override { return _temporal; }
    auto begin() { return _conds.begin(); }
    auto end() { return _conds.end(); }
    auto begin() const { return _conds.begin(); }
    auto end() const { return _conds.end(); }
    auto empty() const -> bool { return _conds.size() == 0; }
    auto singular() const -> bool { return _conds.size() == 1; }
    auto size() const -> size_t { return _conds.size(); }
    auto contains_next() const -> bool override {
        return std::any_of(begin(), end(), [](auto &a) { return a->contains_next(); });
    }
    auto nested_deadlock() const -> bool override;

  protected:
    LogicalCondition() = default;
    auto simplify_or(SimplificationContext &context) const -> retval_t;
    auto simplify_and(SimplificationContext &context) const -> retval_t;

  private:
    virtual auto op() const -> std::string = 0;

  protected:
    bool _temporal = false;
    std::vector<Condition_ptr> _conds;
};

/* Conjunctive and condition */
class AndCondition : public LogicalCondition {
  public:
    AndCondition(std::vector<Condition_ptr> &&conds);

    AndCondition(const std::vector<Condition_ptr> &conds);

    AndCondition(const Condition_ptr &left, const Condition_ptr &right);

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::AND; }
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    auto distance(DistanceContext &context) const -> uint32_t override;

  private:
    // int logicalOp() const;
    auto op() const -> std::string override;
};

/* Disjunctive or conditon */
class OrCondition : public LogicalCondition {
  public:
    OrCondition(std::vector<Condition_ptr> &&conds);

    OrCondition(const std::vector<Condition_ptr> &conds);

    OrCondition(const Condition_ptr &left, const Condition_ptr &right);

    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto get_quantifier() const -> quantifier_e override { return quantifier_e::OR; }
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    auto distance(DistanceContext &context) const -> uint32_t override;

  private:
    // int logicalOp() const;
    auto op() const -> std::string override;
};

class CompareConjunction : public Condition {
  public:
    struct cons_t {
        int32_t _place = -1;
        uint32_t _upper = std::numeric_limits<uint32_t>::max();
        uint32_t _lower = 0;
        std::string _name;
        auto operator<(const cons_t &other) const -> bool { return _place < other._place; }

        void invert() {
            if (_lower == 0 && _upper == std::numeric_limits<uint32_t>::max())
                return;
            assert(_lower == 0 || _upper == std::numeric_limits<uint32_t>::max());
            if (_lower == 0) {
                _lower = _upper + 1;
                _upper = std::numeric_limits<uint32_t>::max();
            } else if (_upper == std::numeric_limits<uint32_t>::max()) {
                _upper = _lower - 1;
                _lower = 0;
            } else {
                assert(false);
            }
        }

        void intersect(const cons_t &other) {
            _lower = std::max(_lower, other._lower);
            _upper = std::min(_upper, other._upper);
        }
    };

    CompareConjunction(bool negated = false){};
    friend FireableCondition;
    CompareConjunction(const std::vector<cons_t> &&cons, bool negated)
        : _constraints(cons), _negated(negated){};
    CompareConjunction(const std::vector<Condition_ptr> &, bool negated);
    CompareConjunction(const CompareConjunction &other, bool negated = false)
        : _constraints(other._constraints), _negated(other._negated != negated){};

    void merge(const CompareConjunction &other);
    void merge(const std::vector<Condition_ptr> &, bool negated);

    auto formula_size() const -> int override {
        int sum = 0;
        for (auto &c : _constraints) {
            assert(c._place >= 0);
            if (c._lower == c._upper)
                ++sum;
            else {
                if (c._lower != 0)
                    ++sum;
                if (c._upper != std::numeric_limits<uint32_t>::max())
                    ++sum;
            }
        }
        if (sum == 1)
            return 2;
        else
            return (sum * 2) + 1;
    }
    void analyze(AnalysisContext &context) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    void to_tapaal_query(std::ostream &stream,
                         TAPAALConditionExportContext &context) const override;
    void to_binary(std::ostream &out) const override;
    auto is_reachability(uint32_t depth) const -> bool override { return depth > 0; };
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::LOPERATOR; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    void to_xml(std::ostream &stream, uint32_t tabs) const override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &visitor) override;

    auto get_quantifier() const -> quantifier_e override {
        return _negated ? quantifier_e::OR : quantifier_e::AND;
    }
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    auto is_negated() const -> bool { return _negated; }
    auto singular() const -> bool {
        return _constraints.size() == 1 &&
               (_constraints[0]._lower == 0 ||
                _constraints[0]._upper == std::numeric_limits<uint32_t>::max());
    };
    auto contains_next() const -> bool override { return false; }
    auto nested_deadlock() const -> bool override { return false; }
    auto constraints() const -> const std::vector<cons_t> & { return _constraints; }
    auto begin() const -> std::vector<cons_t>::const_iterator { return _constraints.begin(); }
    auto end() const -> std::vector<cons_t>::const_iterator { return _constraints.end(); }

  private:
    std::vector<cons_t> _constraints;
    bool _negated = false;
};

/* Comparison conditon */
class CompareCondition : public Condition {
  public:
    CompareCondition(const Expr_ptr &expr1, const Expr_ptr &expr2) : _expr1(expr1), _expr2(expr2) {}

    auto formula_size() const -> int override {
        return _expr1->formula_size() + _expr2->formula_size() + 1;
    }
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    void to_binary(std::ostream &out) const override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::EVAL; }
    auto operator[](uint32_t id) const -> const Expr_ptr & {
        if (id == 0)
            return _expr1;
        else
            return _expr2;
    }
    auto contains_next() const -> bool override { return false; }
    auto is_trivial() const -> bool;
    auto nested_deadlock() const -> bool override { return false; }

    [[nodiscard]] auto get_expr1() const -> const Expr_ptr & { return _expr1; }

    [[nodiscard]] auto get_expr2() const -> const Expr_ptr & { return _expr2; }

  protected:
    auto distance(DistanceContext &c,
                  const std::function<uint32_t(uint32_t, uint32_t, bool)> &d) const -> uint32_t {
        return d(_expr1->evaluate(c), _expr2->evaluate(c), c.negated());
    }

  private:
    virtual auto apply(int v1, int v2) const -> bool = 0;
    virtual auto op() const -> std::string = 0;
    /** Operator when exported to TAPAAL */
    virtual auto op_tapaal() const -> std::string = 0;
    /** Swapped operator when exported to TAPAAL, e.g. operator when operands are swapped */
    virtual auto sop_tapaal() const -> std::string = 0;

  protected:
    Expr_ptr _expr1;
    Expr_ptr _expr2;
};

/* delta */
template <typename T> auto delta(int v1, int v2, bool negated) -> uint32_t { return 0; }

class EqualCondition : public CompareCondition {
  public:
    using CompareCondition::CompareCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto distance(DistanceContext &context) const -> uint32_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto apply(int v1, int v2) const -> bool override;
    auto op() const -> std::string override;
    auto op_tapaal() const -> std::string override;
    auto sop_tapaal() const -> std::string override;
};

/* None equality conditon */
class NotEqualCondition : public CompareCondition {
  public:
    using CompareCondition::CompareCondition;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto distance(DistanceContext &context) const -> uint32_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto apply(int v1, int v2) const -> bool override;
    auto op() const -> std::string override;
    auto op_tapaal() const -> std::string override;
    auto sop_tapaal() const -> std::string override;
};

/* Less-than conditon */
class LessThanCondition : public CompareCondition {
  public:
    using CompareCondition::CompareCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto distance(DistanceContext &context) const -> uint32_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto apply(int v1, int v2) const -> bool override;
    auto op() const -> std::string override;
    auto op_tapaal() const -> std::string override;
    auto sop_tapaal() const -> std::string override;
};

/* Less-than-or-equal conditon */
class LessThanOrEqualCondition : public CompareCondition {
  public:
    using CompareCondition::CompareCondition;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto distance(DistanceContext &context) const -> uint32_t override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;

  private:
    auto apply(int v1, int v2) const -> bool override;
    auto op() const -> std::string override;
    auto op_tapaal() const -> std::string override;
    auto sop_tapaal() const -> std::string override;
};

/* Bool condition */
class BooleanCondition : public Condition {
  public:
    BooleanCondition(bool value) : _value(value) {
        if (value) {
            _trivial = 1;
        } else {
            _trivial = 2;
        }
    }
    auto formula_size() const -> int override { return 0; }
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    static Condition_ptr TRUE_CONSTANT;
    static Condition_ptr FALSE_CONSTANT;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    static auto get_shared(bool val) -> Condition_ptr;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    void to_binary(std::ostream &) const override;

    auto get_quantifier() const -> quantifier_e override { return quantifier_e::EMPTY; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::EVAL; }
    auto contains_next() const -> bool override { return false; }
    auto nested_deadlock() const -> bool override { return false; }
    const bool _value;
};

/* Deadlock condition */
class DeadlockCondition : public Condition {
  public:
    DeadlockCondition() { _loop_sensitive = true; }
    auto formula_size() const -> int override { return 1; }
    void analyze(AnalysisContext &context) override;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;
    void to_binary(std::ostream &) const override;

    static Condition_ptr DEADLOCK;
    auto get_quantifier() const -> quantifier_e override { return quantifier_e::DEADLOCK; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::EVAL; }
    auto contains_next() const -> bool override { return false; }
    auto nested_deadlock() const -> bool override { return false; }
};

class KSafeCondition : public ShallowCondition {
  public:
    KSafeCondition(const Expr_ptr &expr1) : _bound(expr1) {}

    auto get_bound() const -> const Expr_ptr & { return _bound; }

  protected:
    void internal_analyze(AnalysisContext &context) override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto clone() -> Condition_ptr override { return std::make_shared<KSafeCondition>(_bound); }

  private:
    Expr_ptr _bound = nullptr;
};

class LivenessCondition : public ShallowCondition {
  public:
    LivenessCondition() = default;

  protected:
    void internal_analyze(AnalysisContext &context) override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto clone() -> Condition_ptr override { return std::make_shared<LivenessCondition>(); }
};

class QuasiLivenessCondition : public ShallowCondition {
  public:
    QuasiLivenessCondition() = default;

  protected:
    void internal_analyze(AnalysisContext &context) override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto clone() -> Condition_ptr override { return std::make_shared<QuasiLivenessCondition>(); }
};

class StableMarkingCondition : public ShallowCondition {
  public:
    StableMarkingCondition() = default;

  protected:
    void internal_analyze(AnalysisContext &context) override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto clone() -> Condition_ptr override { return std::make_shared<StableMarkingCondition>(); }
};

class UpperBoundsCondition : public ShallowCondition {
  public:
    UpperBoundsCondition(const std::vector<std::string> &places) : _places(places) {}
    auto get_places() const -> const std::vector<std::string> & { return _places; }

  protected:
    void internal_analyze(AnalysisContext &context) override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto clone() -> Condition_ptr override {
        return std::make_shared<UpperBoundsCondition>(_places);
    }

  private:
    std::vector<std::string> _places;
};

class UnfoldedUpperBoundsCondition : public Condition {
  public:
    struct place_t {
        std::string _name;
        uint32_t _place = 0;
        double _max = std::numeric_limits<double>::infinity();
        bool _maxed_out = false;
        place_t(const std::string &name) { _name = name; }
        place_t(const place_t &other, double max) {
            _name = other._name;
            _place = other._place;
            _max = max;
        }
        auto operator<(const place_t &other) const -> bool { return _place < other._place; }
    };

    UnfoldedUpperBoundsCondition(const std::vector<std::string> &places) {
        for (auto &s : places)
            _places.push_back(s);
    }
    UnfoldedUpperBoundsCondition(const std::vector<place_t> &places, double max, double offset)
        : _places(places), _max(max), _offset(offset){};
    UnfoldedUpperBoundsCondition(const UnfoldedUpperBoundsCondition &) = default;
    auto formula_size() const -> int override { return _places.size(); }
    void analyze(AnalysisContext &context) override;
    auto value(const MarkVal *) -> size_t;
    auto evaluate(const EvaluationContext &context) -> result_e override;
    auto eval_and_set(const EvaluationContext &context) -> result_e override;
    void visit(Visitor &) const override;
    void visit(MutatingVisitor &) override;
    auto distance(DistanceContext &context) const -> uint32_t override;
    void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const override;
    void to_binary(std::ostream &) const override;
    auto simplify(SimplificationContext &context) const -> retval_t override;
    auto is_reachability(uint32_t depth) const -> bool override;
    auto prepare_for_reachability(bool negated) const -> Condition_ptr override;
    auto push_negation(negstat_t &, const EvaluationContext &context, bool nested, bool negated,
                       bool initrw) -> Condition_ptr override;
    void to_xml(std::ostream &, uint32_t tabs) const override;

    auto get_quantifier() const -> quantifier_e override { return quantifier_e::UPPERBOUNDS; }
    auto get_path() const -> path_e override { return path_e::P_ERROR; }
    auto get_query_type() const -> ctl_type_e override { return ctl_type_e::EVAL; }
    auto contains_next() const -> bool override { return false; }
    auto nested_deadlock() const -> bool override { return false; }

    auto bounds(bool add_offset = true) const -> double {
        return (add_offset ? _offset : 0) + _bound;
    }

    virtual void set_upper_bound(size_t bound) { _bound = std::max(_bound, bound); }

    auto places() const -> const std::vector<place_t> & { return _places; }

  private:
    std::vector<place_t> _places;
    size_t _bound = 0;
    double _max = std::numeric_limits<double>::infinity();
    double _offset = 0;
};

} // namespace PetriEngine::PQL

#endif // EXPRESSIONS_H
