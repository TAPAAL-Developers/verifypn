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

#include "LTL/LTLToBuchi.h"
#include "LTL/SuccessorGeneration/BuchiSuccessorGenerator.h"
#include "PetriEngine/options.h"

#include <spot/twaalgos/translate.hh>
#include <spot/tl/parse.hh>
#include <spot/twa/bddprint.hh>
#include <sstream>
#include <spot/twaalgos/dot.hh>

using namespace PetriEngine::PQL;

namespace LTL {


    /**
     * Formula serializer to SPOT-compatible syntax.
     */
    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotCondition *element) {
        os << "(! ";
        Visitor::visit(this, (*element)[0]);
        os << ")";
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::AndCondition *element) {
        QueryPrinter::_accept(element, "&&");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::OrCondition *element) {
        QueryPrinter::_accept(element, "||");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanCondition *element) {
        make_atomic_prop(std::make_shared<LessThanCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::UnfoldedFireableCondition *element) {
        make_atomic_prop(std::make_shared<UnfoldedFireableCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::FireableCondition *element) {
        make_atomic_prop(std::make_shared<FireableCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LessThanOrEqualCondition *element) {
        make_atomic_prop(std::make_shared<LessThanOrEqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::EqualCondition *element) {
        make_atomic_prop(std::make_shared<EqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::NotEqualCondition *element) {
        make_atomic_prop(std::make_shared<NotEqualCondition>(*element));
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::CompareConjunction *element) {
        make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::BooleanCondition *element) {
        os << (element->value ? "1" : "0");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::LiteralExpr *element) {
        assert(false);
        throw base_error("LiteralExpr should not be visited by Spot serializer");
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::PlusExpr *element) {
        assert(false);
        throw base_error("PlusExpr should not be visited by Spot serializer");
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MultiplyExpr *element) {
        assert(false);
        throw base_error("MultiplyExpr should not be visited by Spot serializer");
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::MinusExpr *element) {
        assert(false);
        throw base_error("MinusExpr should not be visited by Spot serializer");
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::SubtractExpr *element) {
        assert(false);
        throw base_error("LiteralExpr should not be visited by Spot serializer");
        //make_atomic_prop(element->shared_from_this());
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::IdentifierExpr *element) {
        assert(false);
        throw base_error("IdentifierExpr should not be visited by Spot serializer");
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::ACondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    void FormulaToSpotSyntax::_accept(const PetriEngine::PQL::ECondition *condition) {
        Visitor::visit(this, (*condition)[0]);
    }

    std::pair<spot::formula, APInfo>
    to_spot_formula (const PetriEngine::PQL::Condition_ptr &query, const options_t &options) {
        std::stringstream ss;
        FormulaToSpotSyntax spotConverter{ss, options.ltl_compress_aps};
        Visitor::visit(spotConverter, query);
        std::string spotFormula = ss.str();
        if (spotFormula.at(0) == 'E' || spotFormula.at(0) == 'A') {
            spotFormula = spotFormula.substr(2);
        }
        auto spot_formula = spot::parse_formula(spotFormula);
        return std::make_pair(spot_formula, spotConverter.apInfo());
    }

    Structures::BuchiAutomaton makeBuchiAutomaton(const PetriEngine::PQL::Condition_ptr &query, const options_t &options) {
        auto [formula, apinfo] = to_spot_formula(query, options);
        formula = spot::formula::Not(formula);
        spot::translator translator;
        // Ask for Büchi acceptance (rather than generalized Büchi) and medium optimizations
        // (default is high which causes many worst case BDD constructions i.e. exponential blow-up)
        translator.set_type(spot::postprocessor::BA);
        spot::postprocessor::optimization_level level;
        switch(options.buchiOptimization) {
            case BuchiOptimization::Low:
                level = spot::postprocessor::Low;
                break;
            case BuchiOptimization::Medium:
                level = spot::postprocessor::Medium;
                break;
            case BuchiOptimization::High:
                level = spot::postprocessor::High;
                break;
        }
        translator.set_level(level);
        spot::twa_graph_ptr automaton = translator.run(formula);
        std::unordered_map<int, AtomicProposition> ap_map;
        // bind PQL expressions to the atomic proposition IDs used by spot.
        // the resulting map can be indexed using variables mentioned on edges of the created Büchi automaton.
        for (const auto &info : apinfo) {
            int varnum = automaton->register_ap(info.text);
            ap_map[varnum] = info;
        }

        return Structures::BuchiAutomaton{automaton, ap_map};
    }

    BuchiSuccessorGenerator makeBuchiSuccessorGenerator(const Condition_ptr &query, const options_t &options) {
        return BuchiSuccessorGenerator{makeBuchiAutomaton(query, options)};
    }

}
