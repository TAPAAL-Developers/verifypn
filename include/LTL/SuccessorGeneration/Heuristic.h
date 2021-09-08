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

#ifndef VERIFYPN_HEURISTIC_H
#define VERIFYPN_HEURISTIC_H

#include "LTL/Structures/ProductState.h"
#include "PetriEngine/PetriNet.h"
#include <iostream>

namespace LTL {
class Heuristic {
  public:
    virtual void prepare(const LTL::Structures::ProductState &state) {}

    virtual auto eval(const LTL::Structures::ProductState &state, uint32_t tid) -> uint32_t = 0;

    /**
     * Does the heuristic provide a prioritisation from this state.
     * @return True if a heuristic can be calculated from this state.
     */
    virtual auto has_heuristic(const LTL::Structures::ProductState &) -> bool { return true; }

    virtual void push(uint32_t tid){};

    virtual void pop(uint32_t tid){};

    virtual ~Heuristic() = default;

    virtual auto output(std::ostream &os) -> std::ostream & = 0;
};
} // namespace LTL

#endif // VERIFYPN_HEURISTIC_H
