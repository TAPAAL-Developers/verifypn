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
#ifndef INTERVALGENERATOR_H
#define INTERVALGENERATOR_H

#include "ColoredNetStructures.h"

namespace PetriEngine::Colored {
class IntervalGenerator {
  public:
    IntervalGenerator();
    auto
    get_var_intervals(std::vector<VariableIntervalMap> &variableMaps,
                      const std::unordered_map<uint32_t, arc_intervals_t> &placeArcIntervals) const
        -> bool;

  private:
    [[nodiscard]] auto get_intervals_from_interval(
        const interval_t &interval, uint32_t varPosition, int32_t varModifier,
        const std::vector<const ColorType *> &varColorTypes) const -> std::vector<interval_t>;

    void get_arc_var_intervals(IntervalVector &varIntervals,
                               const std::unordered_map<uint32_t, int32_t> &modIndexMap,
                               const interval_t &interval,
                               const std::vector<const ColorType *> &varColorTypes) const;

    void populate_local_map(const arc_intervals_t &arcIntervals, const VariableIntervalMap &varMap,
                            VariableIntervalMap &localVarMap, const interval_t &interval,
                            bool &allVarsAssigned, uint32_t tuplePos) const;

    void fill_var_maps(std::vector<VariableIntervalMap> &variableMaps,
                       const arc_intervals_t &arcIntervals, const uint32_t &intervalTupleSize,
                       const uint32_t &tuplePos) const;
};
} // namespace PetriEngine::Colored

#endif /* INTERVALGENERATOR_H */