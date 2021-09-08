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

#include "PetriEngine/Colored/IntervalGenerator.h"

namespace PetriEngine::Colored {

IntervalGenerator::IntervalGenerator() = default;

auto IntervalGenerator::get_intervals_from_interval(
    const interval_t &interval, uint32_t varPosition, int32_t varModifier,
    const std::vector<const ColorType *> &varColorTypes) const -> std::vector<interval_t> {
    std::vector<interval_t> varIntervals;
    interval_t firstVarInterval;
    varIntervals.push_back(firstVarInterval);
    for (uint32_t i = varPosition; i < varPosition + varColorTypes.size(); i++) {
        auto ctSize = varColorTypes[i - varPosition]->size();
        int32_t lower_val = ctSize + (interval[i]._lower + varModifier);
        int32_t upper_val = ctSize + (interval[i]._upper + varModifier);
        uint32_t lower = lower_val % ctSize;
        uint32_t upper = upper_val % ctSize;

        if (lower > upper) {
            if (lower == upper + 1) {
                for (auto &varInterval : varIntervals) {
                    varInterval.add_range(0, ctSize - 1);
                }
            } else {
                std::vector<interval_t> newIntervals;
                for (auto &varInterval : varIntervals) {
                    interval_t newVarInterval = varInterval;
                    varInterval.add_range(0, upper);
                    newVarInterval.add_range(lower, ctSize - 1);
                    newIntervals.push_back(newVarInterval);
                }
                varIntervals.insert(varIntervals.end(), newIntervals.begin(), newIntervals.end());
            }
        } else {
            for (auto &varInterval : varIntervals) {
                varInterval.add_range(lower, upper);
            }
        }
    }
    return varIntervals;
}

void IntervalGenerator::get_arc_var_intervals(
    IntervalVector &varIntervals, const std::unordered_map<uint32_t, int32_t> &modIndexMap,
    const interval_t &interval, const std::vector<const ColorType *> &varColorTypes) const {
    for (auto &posModPair : modIndexMap) {
        const auto &intervals = get_intervals_from_interval(interval, posModPair.first,
                                                            posModPair.second, varColorTypes);

        if (varIntervals.empty()) {
            for (auto &interval : intervals) {
                varIntervals.add_interval(interval);
            }
        } else {
            IntervalVector newVarIntervals;
            for (auto &i : varIntervals) {
                auto varInterval = &i;
                for (auto &interval : intervals) {
                    auto overlap = varInterval->get_overlap(interval);
                    if (overlap.is_sound()) {
                        newVarIntervals.add_interval(overlap);
                        // break;
                    }
                }
            }
            varIntervals = newVarIntervals;
        }
    }
}

void IntervalGenerator::populate_local_map(const arc_intervals_t &arcIntervals,
                                           const VariableIntervalMap &varMap,
                                           VariableIntervalMap &localVarMap,
                                           const interval_t &interval, bool &allVarsAssigned,
                                           uint32_t tuplePos) const {
    for (const auto &pair : arcIntervals._varIndexModMap) {
        IntervalVector varIntervals;
        std::vector<const ColorType *> varColorTypes;
        pair.first->_colorType->get_colortypes(varColorTypes);

        get_arc_var_intervals(varIntervals, pair.second[tuplePos], interval, varColorTypes);

        if (arcIntervals._intervalTupleVec.size() > 1 && pair.second[tuplePos].empty()) {
            // The variable is not on this side of the add expression, so we add a full interval to
            // compare against for the other side
            varIntervals.add_interval(pair.first->_colorType->get_full_interval());
        }

        if (varMap.count(pair.first) == 0) {
            localVarMap[pair.first] = varIntervals;
        } else {
            for (const auto &varInterval : varIntervals) {
                for (const auto &interval : varMap.find(pair.first)->second) {
                    auto overlapInterval = varInterval.get_overlap(interval);

                    if (overlapInterval.is_sound()) {
                        localVarMap[pair.first].add_interval(overlapInterval);
                    }
                }
            }
        }

        if (localVarMap[pair.first].empty()) {
            allVarsAssigned = false;
        }
    }
}

void IntervalGenerator::fill_var_maps(std::vector<VariableIntervalMap> &variableMaps,
                                      const arc_intervals_t &arcIntervals,
                                      const uint32_t &intervalTupleSize,
                                      const uint32_t &tuplePos) const {
    for (uint32_t i = 0; i < intervalTupleSize; i++) {
        VariableIntervalMap localVarMap;
        bool validInterval = true;
        const auto &interval = arcIntervals._intervalTupleVec[tuplePos][i];

        for (const auto &pair : arcIntervals._varIndexModMap) {
            IntervalVector varIntervals;
            std::vector<const ColorType *> varColorTypes;
            pair.first->_colorType->get_colortypes(varColorTypes);
            get_arc_var_intervals(varIntervals, pair.second[tuplePos], interval, varColorTypes);

            if (arcIntervals._intervalTupleVec.size() > 1 && pair.second[tuplePos].empty()) {
                // The variable is not on this side of the add expression, so we add a full interval
                // to compare against for the other side
                varIntervals.add_interval(pair.first->_colorType->get_full_interval());
            } else if (varIntervals.size() < 1) {
                // If any varinterval ends up empty then we were unable to use this arc interval
                validInterval = false;
                break;
            }
            localVarMap[pair.first] = varIntervals;
        }

        if (validInterval) {
            variableMaps.push_back(std::move(localVarMap));
        }
    }
}

auto IntervalGenerator::get_var_intervals(
    std::vector<VariableIntervalMap> &variableMaps,
    const std::unordered_map<uint32_t, arc_intervals_t> &placeArcIntervals) const -> bool {
    for (auto &placeArcInterval : placeArcIntervals) {
        for (uint32_t j = 0; j < placeArcInterval.second._intervalTupleVec.size(); j++) {
            uint32_t intervalTupleSize = placeArcInterval.second._intervalTupleVec[j].size();
            // If we have not found intervals for any place yet, we fill the intervals from this
            // place Else we restrict the intervals we already found to only keep those that can
            // also be matched in this place
            if (variableMaps.empty()) {
                fill_var_maps(variableMaps, placeArcInterval.second, intervalTupleSize, j);
            } else {
                std::vector<VariableIntervalMap> newVarMapVec;

                for (const auto &varMap : variableMaps) {
                    for (uint32_t i = 0; i < intervalTupleSize; i++) {
                        VariableIntervalMap localVarMap;
                        bool allVarsAssigned = true;
                        auto interval = placeArcInterval.second._intervalTupleVec[j][i];

                        populate_local_map(placeArcInterval.second, varMap, localVarMap, interval,
                                           allVarsAssigned, j);

                        for (const auto &varTuplePair : varMap) {
                            if (localVarMap.count(varTuplePair.first) == 0) {
                                localVarMap[varTuplePair.first] = varTuplePair.second;
                            }
                        }

                        if (allVarsAssigned) {
                            newVarMapVec.push_back(std::move(localVarMap));
                        }
                    }
                }
                variableMaps = std::move(newVarMapVec);
            }
            // If we did not find any intervals for an arc, then the transition cannot be activated
            if (variableMaps.empty()) {
                return false;
            }
        }
    }
    return true;
}
} // namespace PetriEngine::Colored