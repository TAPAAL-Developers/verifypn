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

#include "PetriEngine/TraceReplay.h"
#include "LTL/LTLToBuchi.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/State.h"
#include "PetriEngine/SuccessorGenerator.h"
#include "errorcodes.h"

#include <cstring>
#include <iostream>
#include <rapidxml.hpp>
#include <vector>

namespace PetriEngine {

TraceReplay::TraceReplay(std::istream &is, const PetriEngine::PetriNet &net,
                         const options_t &options)
    : _options(options) {
    parse(is, net);
}

void TraceReplay::parse(std::istream &xml, const PetriNet &net) {
    _places.clear();
    _transitions.clear();
    // preallocate reverse lookup for transition and place names. Assume this is always called with
    // the same Petri net.
    for (size_t i = 0; i < net.place_names().size(); ++i) {
        _places[net.place_names()[i]] = i;
    }
    for (size_t i = 0; i < net.transition_names().size(); ++i) {
        _transitions[net.transition_names()[i]] = i;
    }
    _transitions[DEADLOCK_TRANS] = -1;

    // TODO can also validate transition names up front.
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(xml)),
                             std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(buffer.data());
    rapidxml::xml_node<> *root = doc.first_node();

    if (root) {
        parse_root(root);
    } else {
        assert(false);
        throw base_error_t("TraceReplay: Error getting root node.");
    }
}

void TraceReplay::parse_root(const rapidxml::xml_node<> *pNode) {
    if (std::strcmp(pNode->name(), "trace") != 0) {
        assert(false);
        throw base_error_t("TraceReplay: Expected trace node. Got: ", pNode->name());
    }
    for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
        if (std::strcmp(it->name(), "loop") == 0)
            _loop_idx = _trace.size();
        else if (std::strcmp(it->name(), "transition") == 0 ||
                 std::strcmp(it->name(), "deadlock") == 0) {
            _trace.push_back(parse_transition(it));
        } else {
            assert(false);
            throw base_error_t("TraceReplay: Expected transition or loop node. Got: ", it->name());
        }
    }
    if (_loop_idx == std::numeric_limits<size_t>::max() &&
        _options._logic == options_t::temporal_logic_e::LTL) {
        assert(false);
        throw base_error_t("TraceReplay: Missing <loop/> statement in trace\n");
    }
}

auto TraceReplay::parse_transition(const rapidxml::xml_node<char> *pNode)
    -> TraceReplay::transition_t {
    std::string id;
    int buchi = -1;
    for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
        if (std::strcmp(it->name(), "id") == 0) {
            id = std::string(it->value());
        }
        if (std::strstr(it->name(), "buchi") != nullptr) {
            // buchi field is sometimes spelled slightly differently, but will always contain
            // 'buchi'
            buchi = std::stoi(it->value());
        }
    }
    if (strcmp(pNode->name(), "deadlock") == 0) {
        id = DEADLOCK_TRANS;
    }
    if (id.empty()) {
        assert(false);
        throw base_error_t("TraceReplay: Transition has no id attribute");
    }

    transition_t transition(id, buchi);

    for (auto it = pNode->first_node(); it; it = it->next_sibling()) {
        if (std::strcmp(it->name(), "token") != 0) {
            std::cerr << "Warning: Unexpected transition child. Expected: token, but got: "
                      << it->name() << std::endl;
        }
        parse_token(it, transition._tokens);
    }

    return transition;
}

void TraceReplay::parse_token(const rapidxml::xml_node<char> *pNode,
                              std::unordered_map<uint32_t, uint32_t> &current_marking) {
    for (auto it = pNode->first_attribute(); it; it = it->next_attribute()) {
        if (std::strcmp(it->name(), "place") == 0) {
            std::string val{it->value()};
            if (current_marking.find(_places.at(val)) == current_marking.end()) {
                current_marking[_places.at(val)] = 1;
            } else {
                ++current_marking[_places.at(val)];
            }
        }
    }
}

auto TraceReplay::replay(const PetriEngine::PetriNet &net,
                         const PetriEngine::PQL::Condition_ptr &cond) -> bool {
    // spot::print_dot(std::cerr, buchiGenerator.aut._buchi);
    PetriEngine::Structures::State state;
    state.set_marking(net.make_initial_marking());
    PetriEngine::SuccessorGenerator successorGenerator(net);

    std::cout << "Playing back trace. Length: " << _trace.size() << std::endl;
    if (play_trace(net, successorGenerator)) {
        std::cout << "Replay complete. No errors" << std::endl;
        return true;
    }
    return false;
}

auto TraceReplay::play_trace(const PetriEngine::PetriNet &net,
                             PetriEngine::SuccessorGenerator &successorGenerator) -> bool {
    PetriEngine::Structures::State state;
    PetriEngine::Structures::State loopstate;
    bool looping = false;
    state.set_marking(net.make_initial_marking());
    loopstate.set_marking(net.make_initial_marking());
    for (size_t i = 0; i < _trace.size(); ++i) {
        const transition_t &transition = _trace[i];
        // looping part should end up at the state _before_ the <loop/> tag,
        // hence copy state from previous iteration.
        if (i == _loop_idx) {
            memcpy(loopstate.marking(), state.marking(), sizeof(uint32_t) * net.number_of_places());
            looping = true;
        }
        successorGenerator.prepare(state);
        auto it = _transitions.find(transition._id);
        if (it == std::end(_transitions)) {
            assert(false);
            throw base_error_t("Unrecognized transition name ", transition._id);
        }
        int tid = it->second;

        if (tid != -1) {
            // fire transition
            if (!successorGenerator.check_preset(tid)) {
                std::cerr << "ERROR the provided trace cannot be replayed on the provided model. "
                             "\nOffending transition: "
                          << transition._id << " at index: " << i << "\n";
                return false;
            }
            successorGenerator.consume_preset(state, tid);
            successorGenerator.produce_postset(state, tid);
        } else {
            // -1 is deadlock, assert deadlocked state.
            // LTL deadlocks are unambiguously in the Petri net, since Büchi deadlocks won't
            // generate any successor in the first place.
            assert(i == _trace.size() - 1);
            if (!net.deadlocked(state.marking())) {
                std::cerr << "ERROR: Trace claimed the net was deadlocked, but there are enabled "
                             "transitions.\n";
                return false;
            }
            return true;
        }

        if (!transition._tokens.empty()) {
            auto [finv, linv] = net.preset(_transitions.at(transition._id));
            for (; finv != linv; ++finv) {
                if (finv->_inhibitor) {

                } else {
                    auto it = transition._tokens.find(finv->_place);
                    if (it == std::end(transition._tokens)) {
                        std::cerr << "ERROR: Pre-place " << net.place_names()[finv->_place]
                                  << " of transition " << transition._id
                                  << " was not mentioned in trace.\n";
                        return false;
                    }
                    if (it->second != finv->_tokens) {
                        std::cerr << "ERROR: Pre-place " << net.place_names()[finv->_place]
                                  << " of transition " << transition._id << "has arc weight "
                                  << finv->_tokens << " but trace consumes " << it->second
                                  << " tokens.\n";
                        return false;
                    }
                }
            }
        }
    }

    bool err = false;
    if (looping) {
        for (size_t i = 0; i < net.number_of_places(); ++i) {
            if (state.marking()[i] != loopstate.marking()[i]) {
                if (!err) {
                    std::cerr << "ERROR end state not equal to expected loop state.\n";
                    err = true;
                }
                std::cerr << "  " << net.place_names()[i] << ": expected" << loopstate.marking()[i]
                          << ", actual " << state.marking()[i] << '\n';
            }
        }
    }

    return !err;
}
} // namespace PetriEngine
