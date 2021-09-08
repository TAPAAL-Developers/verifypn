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

#ifndef VERIFYPN_DISTANCEHEURISTIC_H
#define VERIFYPN_DISTANCEHEURISTIC_H

#include "LTL/SuccessorGeneration/Heuristic.h"

#include "PetriEngine/PQL/Contexts.h"
#include <utility>

namespace LTL {
class DistanceHeuristic : public Heuristic {
  public:
    DistanceHeuristic(const PetriEngine::PetriNet &net, PetriEngine::PQL::Condition_ptr cond)
        : _net(net), _cond(std::move(cond)) {}

    auto eval(const Structures::ProductState &state, uint32_t tid) -> uint32_t override {
        PetriEngine::PQL::DistanceContext context{_net, state.marking()};
        return _cond->distance(context);
    }

    auto output(std::ostream &os) -> std::ostream & override { return os << "DIST_HEUR"; }

  private:
    const PetriEngine::PetriNet &_net;
    const PetriEngine::PQL::Condition_ptr _cond;
};
} // namespace LTL

#endif // VERIFYPN_DISTANCEHEURISTIC_H
