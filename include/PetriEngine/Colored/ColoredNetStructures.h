/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLOREDNETSTRUCTURES_H
#define COLOREDNETSTRUCTURES_H

#include "Colors.h"
#include "Expressions.h"
#include "Multiset.h"
#include <cassert>
#include <set>
#include <vector>

namespace PetriEngine::Colored {

struct arc_t {
    uint32_t _place;
    uint32_t _transition;
    ArcExpression_ptr _expr;
    bool _input;
    uint32_t _weight;
};

struct transition_t {
    std::string _name;
    GuardExpression_ptr _guard;
    std::vector<arc_t> _input_arcs;
    std::vector<arc_t> _output_arcs;
    std::vector<std::unordered_map<const Variable *, IntervalVector>> _variable_maps;
    bool _considered;
};

struct place_t {
    std::string _name;
    const ColorType *_type;
    Multiset _marking;
    bool _inhibitor;
    bool _stable = true;
};
} // namespace PetriEngine::Colored

#endif /* COLOREDNETSTRUCTURES_H */
