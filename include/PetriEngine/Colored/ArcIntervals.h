/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
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

#ifndef ARCINTERVALS_H
#define ARCINTERVALS_H

#include <utility>

#include <utility>

#include "Colors.h"

namespace PetriEngine::Colored {

struct ArcIntervals {
    VariableModifierMap _varIndexModMap;
    std::vector<Colored::interval_vector_t> _intervalTupleVec;
    const Colored::ColorFixpoint *_source{};

    ~ArcIntervals() { _varIndexModMap.clear(); }
    ArcIntervals() = default;

    explicit ArcIntervals(const Colored::ColorFixpoint *source) : _source(source) {}

    ArcIntervals(const Colored::ColorFixpoint *source, VariableModifierMap varIndexModMap)
        : _varIndexModMap(std::move(std::move(varIndexModMap))), _source(source){};

    ArcIntervals(const Colored::ColorFixpoint *source, VariableModifierMap varIndexModMap,
                 std::vector<Colored::interval_vector_t> ranges)
        : _varIndexModMap(std::move(std::move(varIndexModMap))),
          _intervalTupleVec(std::move(std::move(ranges))), _source(source){};

    void print() {
        std::cout << "[ ";
        for (const auto &varModifierPair : _varIndexModMap) {
            std::cout << "(" << varModifierPair.first->_name << ", "
                      << varModifierPair.first->_colorType->product_size() << ") ";
        }
        std::cout << "]" << std::endl;
        for (const auto &intervalTuple : _intervalTupleVec) {
            std::cout << "--------------------------------------------------------------------"
                      << std::endl;
            intervalTuple.print();
            std::cout << "--------------------------------------------------------------------"
                      << std::endl;
        }
    }
};
} // namespace PetriEngine::Colored

#endif /* INTERVALGENERATOR_H */