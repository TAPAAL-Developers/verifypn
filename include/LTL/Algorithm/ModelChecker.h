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

#ifndef VERIFYPN_MODELCHECKER_H
#define VERIFYPN_MODELCHECKER_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/SuccessorGeneration/ProductSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/ReachStubProductSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/ResumingSuccessorGenerator.h"
#include "LTL/SuccessorGeneration/SpoolingSuccessorGenerator.h"
#include "LTL/Structures/BitProductStateSet.h"
#include "LTL/SuccessorGeneration/ReachStubProductSuccessorGenerator.h"
#include "LTL/Structures/ProductStateFactory.h"
#include "options.h"

#include <iomanip>
#include <algorithm>

namespace LTL {
    template<template <typename, typename...> typename ProductSucGen, typename SuccessorGen, typename... Spooler>
    class ModelChecker {
    public:
        ModelChecker(const PetriEngine::PetriNet& net,
                     const PetriEngine::PQL::Condition_ptr &condition,
                     const Structures::BuchiAutomaton &buchi,
                     SuccessorGen *successorGen,
                     std::unique_ptr<Spooler> &&...spooler)
        : _net(net), _formula(condition), _successor_generator(
                std::make_unique<ProductSucGen<SuccessorGen, Spooler...>>(net, buchi, successorGen,
                                                                          std::move(spooler)...)),
        _factory(net, buchi, this->_successor_generator->initial_buchi_state()) {
        }

        void set_options(const options_t &options) {
            _traceLevel = options._trace;
            _shortcircuitweak = options._ltl_use_weak;
            if (_traceLevel != options_t::trace_level_e::None) {
                _maxTransName = 0;
                for (const auto &transname : _net.transition_names()) {
                    _maxTransName = std::max(transname.size(), _maxTransName);
                }
            }
        }

        virtual bool is_satisfied() = 0;

        virtual ~ModelChecker() = default;

        virtual void print_stats(std::ostream &os) = 0;

        [[nodiscard]] bool is_weak() const {
            return _is_weak;
        }

        size_t get_explored() {
            return _stats._explored;
        }

    protected:
        struct stats_t {
            size_t _explored = 0, _expanded = 0;
        };

        stats_t _stats;

        virtual void _print_stats(std::ostream &os, const LTL::Structures::ProductStateSetInterface &stateSet)
        {
            std::cout << "STATS:\n"
                      << "\tdiscovered states: " << stateSet.discovered() << std::endl
                      << "\texplored states:   " << _stats._explored << std::endl
                      << "\texpanded states:   " << _stats._expanded << std::endl
                      << "\tmax tokens:        " << stateSet.max_tokens() << std::endl;
        }

        const PetriEngine::PetriNet& _net;
        PetriEngine::PQL::Condition_ptr _formula;
        std::unique_ptr<ProductSucGen<SuccessorGen, Spooler...>> _successor_generator;

        options_t::trace_level_e _traceLevel;
        LTL::Structures::ProductStateFactory _factory;

        size_t _discovered = 0;
        bool _shortcircuitweak;
        bool _weakskip = false;
        bool _is_weak = false;
        size_t _maxTransName;

        static constexpr auto _indent = "  ";
        static constexpr auto _tokenIndent = "    ";

        void print_loop(std::ostream &os)
{
            os << _indent << "<loop/>\n";
        }

        std::ostream &
        print_transition(size_t transition, LTL::Structures::ProductState &state, std::ostream &os)
        {
            if (transition >= std::numeric_limits<ptrie::uint>::max() - 1) {
                os << _indent << "<deadlock/>";
                return os;
            }
            os << _indent << "<transition id="
                   // field width stuff obsolete without büchi state printing.
                   //<< std::setw(maxTransName + 2) << std::left
                    << std::quoted(_net.transition_names()[transition]);
            if (_traceLevel == options_t::trace_level_e::Full) {
                os << ">";
                os << std::endl;
                auto [fpre, lpre] = _net.preset(transition);
                for(; fpre < lpre; ++fpre) {
                    if (fpre->_inhibitor) {
                        assert(state.marking()[fpre->_place] < fpre->_tokens);
                        continue;
                    }
                    for (size_t i = 0; i < fpre->_tokens; ++i) {
                        assert(state.marking()[fpre->_place] >= fpre->_tokens);
                        os << _tokenIndent << R"(<token age="0" place=")" << _net.place_names()[fpre->_place] << "\"/>\n";
                    }
                }
                /*for (size_t i = 0; i < net->number_of_places(); ++i) {
                    for (size_t j = 0; j < state.marking()[i]; ++j) {
                        os << tokenIndent << R"(<token age="0" place=")" << net->placeNames()[i] << "\"/>\n";
                    }
                }*/
                os << _indent << "</transition>";
            }
            else {
                os << "/>";
            }
            return os;
        }
    };
}

#endif //VERIFYPN_MODELCHECKER_H
