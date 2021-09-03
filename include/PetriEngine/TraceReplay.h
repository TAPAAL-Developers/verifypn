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

#ifndef VERIFYPN_TRACEREPLAY_H
#define VERIFYPN_TRACEREPLAY_H

#include "PetriEngine/PetriNet.h"
#include "options.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/SuccessorGenerator.h"

#include <iostream>
#include <utility>
#include <rapidxml.hpp>

namespace PetriEngine {
    class TraceReplay {
    public:
        TraceReplay(std::istream &is, const PetriEngine::PetriNet& net, const options_t &options);

        struct Token {
            std::string _place;
        };

        struct Transition {
            explicit Transition(std::string id, int buchi) : _id(std::move(id)), _buchi_state(buchi) {}

            std::string _id;
            int _buchi_state;
            std::unordered_map<uint32_t, uint32_t> _tokens;
        };

        void parse(std::istream &xml, const PetriEngine::PetriNet& net);

        bool replay(const PetriEngine::PetriNet& net, const PetriEngine::PQL::Condition_ptr &cond);

        std::vector<Transition> _trace;
    private:

        static constexpr auto _DEADLOCK_TRANS = "##deadlock";
        void parse_root(const rapidxml::xml_node<> *pNode);

        Transition parse_transition(const rapidxml::xml_node<char> *pNode);

        void parse_token(const rapidxml::xml_node<char> *pNode, std::unordered_map<uint32_t, uint32_t> &current_marking);

        size_t _loop_idx = std::numeric_limits<size_t>::max();
        std::unordered_map<std::string, int> _transitions;
        std::unordered_map<std::string, int> _places;
        bool _play_trace(const PetriEngine::PetriNet& net, PetriEngine::SuccessorGenerator &successorGenerator);
        const options_t &_options;
    };
}

#endif //VERIFYPN_TRACEREPLAY_H
