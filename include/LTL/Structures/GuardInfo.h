/* Copyright (C) 2021  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_GUARDINFO_H
#define VERIFYPN_GUARDINFO_H

#include "PetriEngine/PQL/PQL.h"
#include "LTL/Structures/BuchiAutomaton.h"
#include "LTL/Simplification/SpotToPQL.h"

#include <vector>
#include <spot/twa/twagraph.hh>
#include <spot/twa/formula2bdd.hh>



namespace LTL {
    struct GuardInfo {

        struct Guard {
            PetriEngine::PQL::Condition_ptr _condition;
            bdd _decision_diagram;
            uint32_t _dest;

            explicit operator bool () {
                return bool(_condition);
            }
        };

        GuardInfo(size_t buchiState, bool isAccepting) : _buchi_state(buchiState), _is_accepting(isAccepting) {
        }

        int _buchi_state;
        Guard _retarding;
        std::vector<Guard> _progressing;
        bool _is_accepting;


        static std::vector<GuardInfo> from_automaton(const Structures::BuchiAutomaton &aut) {
            std::vector<GuardInfo> state_guards;
            std::vector<AtomicProposition> aps(aut._ap_info.size());
            std::transform(std::begin(aut._ap_info), std::end(aut._ap_info), std::begin(aps),
                           [](const std::pair<int, AtomicProposition> &pair) { return pair.second; });
            for (unsigned state = 0; state < aut._buchi->num_states(); ++state) {
                state_guards.emplace_back(state, aut._buchi->state_is_accepting(state));
                for (auto &e : aut._buchi->out(state)) {
                    auto formula = spot::bdd_to_formula(e.cond, aut._dict);
                    if (e.dst == state) {
                        state_guards.back()._retarding = Guard{toPQL(formula, aps), e.cond, state};
                    } else {
                        state_guards.back()._progressing.push_back(Guard{toPQL(formula, aps), e.cond, e.dst});
                    }
                }
                if (!state_guards.back()._retarding) {
                    state_guards.back()._retarding = Guard{std::make_shared<PetriEngine::PQL::BooleanCondition>(false), bddfalse, state};
                }
            }
            return state_guards;
        }
    };
}

#endif //VERIFYPN_GUARDINFO_H
