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
#ifndef GENERALSTATE_H
#define GENERALSTATE_H

#include "../PetriNet.h"

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace PetriEngine {
namespace Structures {

/** GeneralState class for reachability searches.
 * Used in most reachability search cases */
class State {
  public:
    MarkVal *marking() { return _marking; }

    const MarkVal *marking() const { return _marking; }

    void set_marking(MarkVal *m) { _marking = m; }

    State() { _marking = nullptr; }

    State(const State &state) = delete;

    State(State &&state) : _marking(state._marking) { state._marking = nullptr; }

    State &operator=(State &&other) {
        if (&other != this) {
            delete _marking;
            _marking = other._marking;
            other._marking = nullptr;
        }
        return *this;
    }

    virtual ~State() { delete[] _marking; }

    void swap(State &other) { std::swap(_marking, other._marking); }

    void print(const PetriNet &net, std::ostream &os = std::cout) {
        for (uint32_t i = 0; i < net.number_of_places(); i++) {
            if (_marking[i])
                os << net.place_names()[i] << ": " << _marking[i] << std::endl;
        }
        os << std::endl;
    }

    std::ostream &print_short(const PetriNet &net, std::ostream &os) {
        for (uint32_t i = 0; i < net.number_of_places(); i++) {
            os << _marking[i];
        }
        return os;
    }

    bool operator==(const State &rhs) const { return _marking == rhs._marking; }

    bool operator!=(const State &rhs) const { return !(rhs == *this); }

  private:
    MarkVal *_marking;
};

} // namespace Structures
} // namespace PetriEngine

#endif // GENERALSTATE_H
