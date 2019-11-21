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
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <memory>

#ifdef ENABLE_TAR
#include <z3++.h>
#endif

#include "../PetriNet.h"
#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "PetriEngine/Simplification/LPCache.h"
#include "ExprError.h"

namespace PetriEngine {
    class ReducingSuccessorGenerator;
    namespace Simplification
    {
        class Member;
        struct Retval;
    }
    namespace PQL {
        
        enum CTLType {PATHQEURY = 1, LOPERATOR = 2, EVAL = 3, TYPE_ERROR = -1};
        enum Quantifier { AND = 1, OR = 2, A = 3, E = 4, NEG = 5, COMPCONJ = 6, DEADLOCK = 7, UPPERBOUNDS = 8, PN_BOOLEAN = 9, EMPTY = -1 };
        enum Path { G = 1, X = 2, F = 3, U = 4, pError = -1 };
        
        
        class AnalysisContext;
        class EvaluationContext;
        class DistanceContext;
        class TAPAALConditionExportContext;
        class SimplificationContext;
        enum Result {RUNKNOWN=-1,RFALSE=0,RTRUE=1};
        using StableResult = std::pair<Result,bool>;
        struct bounds_t {
            explicit bounds_t(int64_t v)
            {
                value = lower = upper = v;
            }
            bounds_t(int64_t v, int64_t l, int64_t u)
            {
                value = v;
                lower = l;
                upper = u;
            }
            
            bool stable() const {
                return lower == upper;
            }
            
            bounds_t operator+(const bounds_t& other) const;
            bounds_t operator*(const bounds_t& other) const;
            bounds_t operator-(const bounds_t& other) const;
            
            StableResult operator==(const bounds_t& other) const;
            StableResult operator!=(const bounds_t& other) const;
            StableResult operator<=(const bounds_t& other) const;
            StableResult operator>=(const bounds_t& other) const;
            StableResult operator<(const bounds_t& other) const;
            StableResult operator>(const bounds_t& other) const;

            
            int64_t value;
            int64_t lower;
            int64_t upper;
        };
        /** Representation of an expression */
        class Expr {
            bounds_t _eval = bounds_t(0);
        public:
            /** Types of expressions */
            enum Types {
                /** Binary addition expression */
                PlusExpr,
                /** Binary subtraction expression */
                SubtractExpr,
                /** Binary multiplication expression */
                MultiplyExpr,
                /** Unary minus expression */
                MinusExpr,
                /** Literal integer expression */
                LiteralExpr,
                /** Identifier expression */
                IdentifierExpr
            };
        public:
            /** Virtual destructor, an expression should know it subexpressions */
            virtual ~Expr();
            /** Perform context analysis */
            virtual void analyze(AnalysisContext& context) = 0;
            /** Evaluate the expression given marking and assignment */
            virtual bounds_t evaluate(const EvaluationContext& context) = 0;
            bounds_t evalAndSet(const EvaluationContext& context);
#ifdef ENABLE_TAR
            virtual z3::expr encodeSat(const PetriNet& net, z3::context& context, std::vector<int32_t>& uses, std::vector<bool>& incremented) const = 0;
#endif
            /** Generate LLVM intermediate code for this expr  */
            //virtual llvm::Value* codegen(CodeGenerationContext& context) const = 0;
            /** Convert expression to string */
            virtual void toString(std::ostream&) const = 0;
            /** Expression type */
            virtual Types type() const = 0;
            /** Construct left/right side of equations used in query simplification */
            virtual Simplification::Member constraint(SimplificationContext& context) const = 0;
            /** Output the expression as it currently is to a file in XML */
            virtual void toXML(std::ostream&, uint32_t tabs, bool tokencount = false) const = 0;
            virtual void toBinary(std::ostream&) const = 0;
            /** Stubborn reduction: increasing and decreasing sets */
            virtual void incr(ReducingSuccessorGenerator& generator) const = 0;
            virtual void decr(ReducingSuccessorGenerator& generator) const = 0;
            /** Count size of the entire formula in number of nodes */
            virtual int formulaSize() const = 0;
            
            virtual bool placeFree() const = 0;
            
            void setEval(bounds_t eval) {
                _eval = eval;
            }
            
            bounds_t getEval() {
                return _eval;
            }
        };
/******************* NEGATION PUSH STATS  *******************/
        
        struct negstat_t
        {
            negstat_t()
            {
                for(size_t i = 0; i < 31; ++i) _used[i] = 0;
            }
            void print(std::ostream& stream)
            {
                for(size_t i = 0; i < 31; ++i) stream << _used[i] << ",";                
            }
            void printRules(std::ostream& stream)
            {
                for(size_t i = 0; i < 31; ++i) stream << _rulename[i] << ",";                
            }
            int _used[31];
            const std::vector<const char*> _rulename = {
                "EG p-> !EF !p",
                "AG p-> !AF !p",
                "!EX p -> AX p",
                "EX false -> false",
                "EX true -> !deadlock",
                "!AX p -> EX p",
                "AX false -> deadlock",
                "AX true -> true",             
                "EF !deadlock -> !deadlock",
                "EF EF p -> EF p",
                "EF AF p -> AF p",
                "EF E p U q -> EF q",
                "EF A p U q -> EF q",
                "EF .. or .. -> EF .. or EF ..",
                "AF !deadlock -> !deadlock",
                "AF AF p -> AF p",
                "AF EF p -> EF p",
                "AF .. or EF p -> EF p or AF ..",
                "AF A p U q -> AF q",
                "A p U !deadlock -> !deadlock",
                "A deadlock U q -> q",
                "A !deadlock U q -> AF q",
                "A p U AF q -> AF q",
                "A p U EF q -> EF q",
                "A p U .. or EF q -> EF q or A p U ..",
                "E p U !deadlock -> !deadlock",
                "E deadlock U q -> q",
                "E !deadlock U q -> EF q",
                "E p U EF q -> EF q",
                "E p U .. or EF q -> EF q or E p U ..",
                "!! p -> p"
                
            };
            int& operator[](size_t i) { return _used[i]; }
            bool negated_fireability = false;
        };
        
        /** Base condition */
        class Condition {
        private:
            StableResult _eval = StableResult(RUNKNOWN,false);
            bool _inv = false;
        protected:
            bool _loop_sensitive = false;            
        public:
            /** Virtual destructor */
            virtual ~Condition();
            /** Perform context analysis  */
            virtual void analyze(AnalysisContext& context) = 0;
            /** Evaluate condition */
            virtual StableResult evaluate(const EvaluationContext& context) = 0;
            virtual StableResult evalAndSet(const EvaluationContext& context) = 0;
            
#ifdef ENABLE_TAR
            virtual z3::expr encodeSat(const PetriNet& net, z3::context& context, std::vector<int32_t>& uses, std::vector<bool>& incremented) const = 0;
#endif
            
            /** Generate LLVM intermediate code for this condition  */
            //virtual llvm::Value* codegen(CodeGenerationContext& context) const = 0;
            /** Convert condition to string */
            virtual void toString(std::ostream&) const = 0;
            /** Export condition to TAPAAL query (add EF manually!) */
            virtual void toTAPAALQuery(std::ostream&, TAPAALConditionExportContext& context) const = 0;
            /** Get distance to query */
            virtual uint32_t distance(DistanceContext& context) const = 0;
            /** Query Simplification */
            virtual Simplification::Retval simplify(SimplificationContext& context) const = 0;
            /** Check if query is a reachability query */
            virtual bool isReachability(uint32_t depth = 0) const = 0;

            virtual bool isLoopSensitive() const { return _loop_sensitive; };
            /** Prepare reachability queries */
            virtual std::shared_ptr<Condition> prepareForReachability(bool negated = false) const = 0;
            virtual std::shared_ptr<Condition> pushNegation(negstat_t&, const EvaluationContext& context, bool nested, bool negated = false, bool initrw = true) = 0;
            
            /** Output the condition as it currently is to a file in XML */
            virtual void toXML(std::ostream&, uint32_t tabs) const = 0;
            virtual void toBinary(std::ostream& out) const = 0;

            /** Find interesting transitions in stubborn reduction*/
            virtual void findInteresting(ReducingSuccessorGenerator& generator, bool negated) const = 0;
            /** Checks if the condition is trivially true */
            bool isTriviallyTrue();
            /*** Checks if the condition is trivially false */
            bool isTriviallyFalse();
            /** Count size of the entire formula in number of nodes */
            virtual int formulaSize() const = 0;

            bool isSatisfied() const
            {
                return _eval.first == RTRUE;
            }
            
            StableResult getEval() const
            { 
                return _eval;
            }
            
            void setSatisfied(StableResult isSatisfied)
            {
                _eval = isSatisfied;
            }
            
            void setInvariant(bool isInvariant)
            {
                _inv = isInvariant;
            }
           
            bool isInvariant() const
            {
                return _inv;
            }
            
            virtual bool isTemporal() const { return false;}
            virtual CTLType getQueryType() const = 0;
            virtual Quantifier getQuantifier() const = 0;
            virtual Path getPath() const = 0;
            static std::shared_ptr<Condition> 
            initialMarkingRW(std::function<std::shared_ptr<Condition> ()> func, negstat_t& stats, const EvaluationContext& context, bool nested, bool negated, bool initrw);
            virtual bool containsNext() const = 0;   
            virtual bool nestedDeadlock() const = 0;
        protected:
            //Value for checking if condition is trivially true or false.
            //0 is undecided (default), 1 is true, 2 is false.
            uint32_t trivial = 0;
        };
        typedef std::shared_ptr<Condition> Condition_ptr;
        typedef std::shared_ptr<Expr> Expr_ptr;
    } // PQL
} // PetriEngine

#endif // PQL_H
