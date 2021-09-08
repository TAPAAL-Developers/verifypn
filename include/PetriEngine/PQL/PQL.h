/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef PQL_H
#define PQL_H
#include <algorithm>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../PetriNet.h"
//#include "../Structures/State.h"
//#include "../ReducingSuccessorGenerator.h"

namespace PetriEngine {
class ReducingSuccessorGenerator;
class StubbornSet;
namespace Simplification {
class Member;
struct retval_t;
} // namespace Simplification
namespace PQL {
class Visitor;
class MutatingVisitor;

enum ctl_type_e { PATHQEURY = 1, LOPERATOR = 2, EVAL = 3, TYPE_ERROR = -1 };
enum quantifier_e {
    AND = 1,
    OR = 2,
    A = 3,
    E = 4,
    NEG = 5,
    COMPCONJ = 6,
    DEADLOCK = 7,
    UPPERBOUNDS = 8,
    PN_BOOLEAN = 9,
    EMPTY = -1
};
enum path_e { G = 1, X = 2, F = 3, U = 4, P_ERROR = -1 };

class AnalysisContext;
class EvaluationContext;
class DistanceContext;
class TAPAALConditionExportContext;
class SimplificationContext;

/** Representation of a PQL error */
class ExprError {
    std::string _text;
    int _length;

  public:
    ExprError(std::string text = "", int length = 0) {
        _text = std::move(text);
        _length = length;
    }

    /** Human readable explaination of the error */
    [[nodiscard]] auto text() const -> const std::string & { return _text; }

    /** length in the source, 0 if not applicable */
    [[nodiscard]] auto length() const -> int { return _length; }

    /** Convert error to string */
    [[nodiscard]] auto to_string() const -> std::string {
        return "Parsing error \"" + text() + "\"";
    }

    /** True, if this is a default created ExprError without any information */
    [[nodiscard]] auto is_empty() const -> bool { return _text.empty() && _length == 0; }
};

/** Representation of an expression */
class Expr {
    int _eval = 0;

  public:
    /** Types of expressions */
    enum types_e {
        /** Binary addition expression */
        PLUS_EXPR,
        /** Binary subtraction expression */
        SUBTRACT_EXPR,
        /** Binary multiplication expression */
        MULTIPLY_EXPR,
        /** Unary minus expression */
        MINUS_EXPR,
        /** Literal integer expression */
        LITERAL_EXPR,
        /** Identifier expression */
        IDENTIFIER_EXPR
    };

  public:
    /** Virtual destructor, an expression should know it subexpressions */
    virtual ~Expr();
    /** Perform context analysis */
    virtual void analyze(AnalysisContext &context) = 0;
    /** Evaluate the expression given marking and assignment */
    [[nodiscard]] virtual auto evaluate(const EvaluationContext &context) -> int = 0;
    auto eval_and_set(const EvaluationContext &context) -> int;
    virtual void visit(Visitor &visitor) const = 0;
    /** Expression type */
    [[nodiscard]] virtual types_e type() const = 0;
    /** Construct left/right side of equations used in query simplification */
    virtual auto constraint(SimplificationContext &context) const -> Simplification::Member = 0;
    /** Output the expression as it currently is to a file in XML */
    virtual void to_xml(std::ostream &, uint32_t tabs, bool tokencount = false) const = 0;
    virtual void to_binary(std::ostream &) const = 0;

    /** Count size of the entire formula in number of nodes */
    [[nodiscard]] virtual auto formula_size() const -> int = 0;

    [[nodiscard]] virtual auto place_free() const -> bool = 0;

    void set_eval(int eval) { _eval = eval; }

    [[nodiscard]] auto get_eval() const -> int { return _eval; }
};
/******************* NEGATION PUSH STATS  *******************/

struct negstat_t {
    static constexpr std::array _rulename{
        "EG p-> !EF !p", "AG p-> !AF !p", "!EX p -> AX p", "EX false -> false",
        "EX true -> !deadlock", "!AX p -> EX p", "AX false -> deadlock", "AX true -> true",
        "EF !deadlock -> !deadlock", "EF EF p -> EF p", "EF AF p -> AF p", "EF E p U q -> EF q",
        "EF A p U q -> EF q", "EF .. or .. -> EF .. or EF ..", "AF !deadlock -> !deadlock",
        "AF AF p -> AF p", "AF EF p -> EF p", "AF .. or EF p -> EF p or AF ..",
        "AF A p U q -> AF q", "A p U !deadlock -> !deadlock", "A deadlock U q -> q",
        "A !deadlock U q -> AF q", "A p U AF q -> AF q", "A p U EF q -> EF q",
        "A p U .. or EF q -> EF q or A p U ..", "E p U !deadlock -> !deadlock",
        "E deadlock U q -> q", "E !deadlock U q -> EF q", "E p U EF q -> EF q",
        "E p U .. or EF q -> EF q or E p U ..", "!! p -> p",
        // LTL rules
        "F F p -> F p", "F p U q -> F q", "F p or q -> F p or F q", "p U F q -> F q"};
    static constexpr size_t nrules = std::tuple_size<decltype(_rulename)>::value;

    negstat_t() {
        for (int &i : _used)
            i = 0;
    }
    void print(std::ostream &stream) {
        for (int i : _used)
            stream << i << ",";
    }

    static inline void print_rules(std::ostream &stream) {
        for (size_t i = 0; i < nrules; ++i)
            stream << _rulename[i] << ",";
    }

    int _used[nrules];
    auto operator[](size_t i) -> int & { return _used[i]; }
    bool _negated_fireability = false;
};

/** Base condition */
class Condition : public std::enable_shared_from_this<Condition> {
  public:
    enum result_e { RUNKNOWN = -1, RFALSE = 0, RTRUE = 1 };

  private:
    bool _inv = false;
    result_e _eval = RUNKNOWN;

  protected:
    bool _loop_sensitive = false;

  public:
    /** Virtual destructor */
    virtual ~Condition();
    /** Perform context analysis  */
    virtual void analyze(AnalysisContext &context) = 0;
    /** Evaluate condition */
    virtual result_e evaluate(const EvaluationContext &context) = 0;
    virtual result_e eval_and_set(const EvaluationContext &context) = 0;
    virtual void visit(Visitor &visitor) const = 0;
    virtual void visit(MutatingVisitor &visitor) = 0;

    /** Export condition to TAPAAL query (add EF manually!) */
    virtual void to_tapaal_query(std::ostream &, TAPAALConditionExportContext &context) const = 0;
    /** Get distance to query */
    [[nodiscard]] virtual auto distance(DistanceContext &context) const -> uint32_t = 0;
    /** Query Simplification */
    virtual auto simplify(SimplificationContext &context) const -> Simplification::retval_t = 0;
    /** Check if query is a reachability query */
    [[nodiscard]] virtual auto is_reachability(uint32_t depth = 0) const -> bool = 0;

    [[nodiscard]] virtual auto is_loop_sensitive() const -> bool { return _loop_sensitive; };
    /** Prepare reachability queries */
    [[nodiscard]] virtual auto prepare_for_reachability(bool negated = false) const
        -> std::shared_ptr<Condition> = 0;
    [[nodiscard]] virtual auto push_negation(negstat_t &, const EvaluationContext &context,
                                             bool nested, bool negated = false, bool initrw = true)
        -> std::shared_ptr<Condition> = 0;

    /** Output the condition as it currently is to a file in XML */
    virtual void to_xml(std::ostream &, uint32_t tabs) const = 0;
    virtual void to_binary(std::ostream &out) const = 0;

    /** Checks if the condition is trivially true */
    [[nodiscard]] auto is_trivially_true() -> bool;
    /*** Checks if the condition is trivially false */
    [[nodiscard]] auto is_trivially_false() -> bool;
    /** Count size of the entire formula in number of nodes */
    [[nodiscard]] virtual auto formula_size() const -> int = 0;

    [[nodiscard]] auto is_satisfied() const -> bool { return _eval == RTRUE; }

    void set_satisfied(bool isSatisfied) { _eval = isSatisfied ? RTRUE : RFALSE; }

    void set_satisfied(result_e isSatisfied) { _eval = isSatisfied; }

    [[nodiscard]] result_e get_satisfied() const { return _eval; }

    void set_invariant(bool isInvariant) { _inv = isInvariant; }

    auto is_invariant() const -> bool { return _inv; }

    [[nodiscard]] virtual auto is_temporal() const -> bool { return false; }
    [[nodiscard]] virtual ctl_type_e get_query_type() const = 0;
    [[nodiscard]] virtual quantifier_e get_quantifier() const = 0;
    [[nodiscard]] virtual path_e get_path() const = 0;
    [[nodiscard]] static auto
    initial_marking_rewrite(const std::function<std::shared_ptr<Condition>()> &func,
                            negstat_t &stats, const EvaluationContext &context, bool nested,
                            bool negated, bool initrw) -> std::shared_ptr<Condition>;
    [[nodiscard]] virtual auto contains_next() const -> bool = 0;
    [[nodiscard]] virtual auto nested_deadlock() const -> bool = 0;
    void to_string(std::ostream &os = std::cout);

  protected:
    // Value for checking if condition is trivially true or false.
    // 0 is undecided (default), 1 is true, 2 is false.
    uint32_t _trivial = 0;
};
using Condition_ptr = std::shared_ptr<Condition>;
using Condition_constptr = std::shared_ptr<const Condition>;
using Expr_ptr = std::shared_ptr<Expr>;
} // namespace PQL
} // namespace PetriEngine

#endif // PQL_H
