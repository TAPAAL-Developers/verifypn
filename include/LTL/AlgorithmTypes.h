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

#ifndef VERIFYPN_ALGORITHMTYPES_H
#define VERIFYPN_ALGORITHMTYPES_H

#include "errorcodes.h"
#include <cassert>

namespace LTL {
enum class algorithm_e { NDFS, TARJAN, NONE = -1 };

enum class buchi_out_type_e { DOT, HOA, SPIN };

inline auto to_string(algorithm_e alg) {
    switch (alg) {
    case algorithm_e::NDFS:
        return "NDFS";
    case algorithm_e::TARJAN:
        return "TARJAN";
    case algorithm_e::NONE:
    default:
        throw base_error_t("to_string: Invalid LTL Algorithm ", static_cast<int>(alg));
        assert(false);
        return "None";
    }
}
} // namespace LTL

#endif // VERIFYPN_ALGORITHMTYPES_H
