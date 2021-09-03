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

#include "LTL/LTL.h"
#include "LTL/LTLMain.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"

#include "LTL/SuccessorGeneration/Spoolers.h"
#include "LTL/SuccessorGeneration/Heuristics.h"
#include "LTL/SuccessorGeneration/HeuristicParser.h"

#include <utility>

using namespace PetriEngine::PQL;
using namespace PetriEngine;

// Causes FORMULA <qname> STATS EXPLORED <nexplored> to be printed after verification. Remove if this causes a nuisance.
// #define DEBUG_EXPLORED_STATES

namespace LTL {
    struct Result {
        bool satisfied = false;
        bool is_weak = true;
        Algorithm algorithm = Algorithm::Tarjan;
#ifdef DEBUG_EXPLORED_STATES
        size_t explored_states = 0;
#endif
    };

/**
 * Converts a formula on the form A f, E f or f into just f, assuming f is an LTL formula.
 * In the case E f, not f is returned, and in this case the model checking result should be negated
 * (indicated by bool in return value)
 * @param formula - a formula on the form A f, E f or f
 * @return @code(ltl_formula, should_negate) - ltl_formula is the formula f if it is a valid LTL formula, nullptr otherwise.
 * should_negate indicates whether the returned formula is negated (in the case the parameter was E f)
 */
    std::pair<Condition_ptr, bool> to_ltl(const Condition_ptr &formula)
    {
        LTL::LTLValidator validator;
        bool should_negate = false;
        Condition_ptr converted;
        if (auto _formula = dynamic_cast<ECondition *>(formula.get())) {
            converted = std::make_shared<NotCondition>((*_formula)[0]);
            should_negate = true;
        } else if (auto _formula = dynamic_cast<ACondition *>(formula.get())) {
            converted = (*_formula)[0];
        } else {
            converted = formula;
        }
        converted->visit(validator);
        if (validator.bad()) {
            converted = nullptr;
        }
        return std::make_pair(converted, should_negate);
    }

    template<typename Checker>
    Result _verify(std::unique_ptr<Checker> checker,
                   const options_t &options)
    {
        Result result;
        checker->set_options(options);
        result.satisfied = checker->is_satisfied();
        result.is_weak = checker->is_weak();
#ifdef DEBUG_EXPLORED_STATES
        result.explored_states = checker->get_explored();
#endif
        if (options._print_statistics) {
            checker->print_stats(std::cout);
        }
        return result;
    }

    std::unique_ptr<Heuristic> make_heuristic(const PetriNet *net,
                                              const Condition_ptr &negated_formula,
                                              const Structures::BuchiAutomaton& automaton,
                                              options_t &options) {
        if (options._strategy == options_t::search_strategy_e::RDFS) {
            return std::make_unique<RandomHeuristic>(options.seed());
        }
        if (options._strategy != options_t::search_strategy_e::HEUR && options._strategy != options_t::search_strategy_e::DEFAULT) {
            return nullptr;
        }
        auto heur = ParseHeuristic(net, automaton, negated_formula, options._ltl_heuristic);
        if (heur == nullptr) {
            std::cerr << "Invalid heuristic specification, terminating.\n";
            exit(1);
        }
        else return heur;
    }

    bool LTLMain(const PetriNet *net,
                        const Condition_ptr &query,
                        const std::string &queryName,
                        options_t &options)
    {

        // force AP compress off for Büchi prints
        options._ltl_compress_aps = options._buchi_out_file.empty() ? options._ltl_compress_aps : options_t::atomic_compression_e::None;

        auto [negated_formula, negate_answer] = to_ltl(query);

        Structures::BuchiAutomaton automaton = make_buchi_automaton(negated_formula, options);
        if (!options._buchi_out_file.empty()) {
            automaton.output_buchi(options._buchi_out_file, options._buchi_out_type);
        }

        bool is_visible_stub = options._stubborn_reduction
                               && (options._ltl_por == options_t::ltl_partial_order_e::Visible ||
                                   options._ltl_por == options_t::ltl_partial_order_e::VisibleReach)
                               && !net->has_inhibitor()
                               && !negated_formula->containsNext();
        bool is_autreach_stub = options._stubborn_reduction
                && (options._ltl_por == options_t::ltl_partial_order_e::AutomatonReach ||
                    options._ltl_por == options_t::ltl_partial_order_e::VisibleReach)
                && !net->has_inhibitor();
        bool is_buchi_stub = options._stubborn_reduction
                && options._ltl_por == options_t::ltl_partial_order_e::FullAutomaton
                && !net->has_inhibitor();

        bool is_stubborn = options._ltl_por != options_t::ltl_partial_order_e::None && (is_visible_stub || is_autreach_stub || is_buchi_stub);

        std::unique_ptr<SuccessorSpooler> spooler;
        std::unique_ptr<Heuristic> heuristic = make_heuristic(net, negated_formula, automaton, options);

        Result result;
        switch (options._ltl_algorithm) {
            case Algorithm::NDFS:
                if (options._strategy != options_t::search_strategy_e::DFS) {
                    SpoolingSuccessorGenerator gen{net, negated_formula};
                    spooler = std::make_unique<EnabledSpooler>(net, gen);
                    gen.set_spooler(spooler.get());
                    gen.set_heuristic(heuristic.get());
                    result = _verify(
                            std::make_unique<NestedDepthFirstSearch<SpoolingSuccessorGenerator>>(
                                    net, negated_formula, automaton, &gen, options._trace != options_t::trace_level_e::None),
                            options);

                } else {
                    ResumingSuccessorGenerator gen{net};
                    result = _verify(
                            std::make_unique<NestedDepthFirstSearch<ResumingSuccessorGenerator>>(
                                    net, negated_formula, automaton, &gen, options._trace != options_t::trace_level_e::None),
                            options);
                }
                break;

            case Algorithm::Tarjan:
                if (options._strategy != options_t::search_strategy_e::DFS || is_stubborn) {
                    // Use spooling successor generator in case of different search strategy or stubborn set method.
                    // Running default, BestFS, or RDFS search strategy so use spooling successor generator to enable heuristics.
                    SpoolingSuccessorGenerator gen{net, negated_formula};
                    if (is_visible_stub) {
                        spooler = std::make_unique<VisibleLTLStubbornSet>(*net, negated_formula);
                    } else if (is_buchi_stub) {
                        spooler = std::make_unique<AutomatonStubbornSet>(*net, automaton);
                    }
                    else {
                        spooler = std::make_unique<EnabledSpooler>(net, gen);
                    }

                    assert(spooler);
                    gen.set_spooler(spooler.get());
                    // if search strategy used, set heuristic, otherwise ignore it
                    // (default is null which is checked elsewhere)
                    if (options._strategy != options_t::search_strategy_e::DFS) {
                        assert(heuristic != nullptr);
                        gen.set_heuristic(heuristic.get());
                    }

                    if (options._trace != options_t::trace_level_e::None) {
                        if (is_autreach_stub && is_visible_stub) {
                            result = _verify(std::make_unique<TarjanModelChecker<ReachStubProductSuccessorGenerator, SpoolingSuccessorGenerator, true, VisibleLTLStubbornSet>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen,
                                                     std::make_unique<VisibleLTLStubbornSet>(*net, negated_formula)),
                                             options);
                        }
                        else if (is_autreach_stub && !is_visible_stub) {
                            result = _verify(std::make_unique<TarjanModelChecker<ReachStubProductSuccessorGenerator, SpoolingSuccessorGenerator, true, EnabledSpooler>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen,
                                                     std::make_unique<EnabledSpooler>(net, gen)),
                                             options);
                        }
                        else {
                            result = _verify(std::make_unique<TarjanModelChecker<ProductSuccessorGenerator, SpoolingSuccessorGenerator, true>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen),
                                             options);
                        }
                    } else {

                        if (is_autreach_stub && is_visible_stub) {
                            result = _verify(std::make_unique<TarjanModelChecker<ReachStubProductSuccessorGenerator, SpoolingSuccessorGenerator, false, VisibleLTLStubbornSet>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen,
                                                     std::make_unique<VisibleLTLStubbornSet>(*net, negated_formula)),
                                             options);
                        } else if (is_autreach_stub && !is_visible_stub) {
                            result = _verify(std::make_unique<TarjanModelChecker<ReachStubProductSuccessorGenerator, SpoolingSuccessorGenerator, false, EnabledSpooler>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen,
                                                     std::make_unique<EnabledSpooler>(net, gen)),
                                             options);
                        }
                        else {
                            result = _verify(std::make_unique<TarjanModelChecker<ProductSuccessorGenerator, SpoolingSuccessorGenerator, false>>(
                                                     net,
                                                     negated_formula,
                                                     automaton,
                                                     &gen),
                                             options);
                        }
                    }
                } else {
                    ResumingSuccessorGenerator gen{net};

                    // no spooling needed, thus use resuming successor generation
                    if (options._trace != options_t::trace_level_e::None) {
                        result = _verify(std::make_unique<TarjanModelChecker<ProductSuccessorGenerator, ResumingSuccessorGenerator, true>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 &gen),
                                         options);
                    } else {
                        result = _verify(std::make_unique<TarjanModelChecker<ProductSuccessorGenerator, ResumingSuccessorGenerator, false>>(
                                                 net,
                                                 negated_formula,
                                                 automaton,
                                                 &gen),
                                         options);
                    }
                }
                break;
            case Algorithm::None:
                assert(false);
                std::cerr << "Error: cannot LTL verify with algorithm None";
        }
        std::cout << "FORMULA " << queryName
                  << (result.satisfied ^ negate_answer ? " TRUE" : " FALSE") << " TECHNIQUES EXPLICIT "
                  << LTL::to_string(options._ltl_algorithm)
                  << (result.is_weak ? " WEAK_SKIP" : "")
                  << (is_stubborn ? " STUBBORN" : "")
                  << (is_visible_stub ? " CLASSIC_STUB" : "")
                  << (is_autreach_stub ? " REACH_STUB" : "")
                  << (is_buchi_stub ? " BUCHI_STUB" : "");
        if (heuristic != nullptr) {
            std::cout << " HEURISTIC ";
            heuristic->output(std::cout);
        }
        std::cout << " OPTIM-" << static_cast<int>(options._buchi_optimization) << std::endl;
#ifdef DEBUG_EXPLORED_STATES
        std::cout << "FORMULA " << queryName << " STATS EXPLORED " << result.explored_states << std::endl;
#endif
        return result.satisfied ^ negate_answer;
    }
}
