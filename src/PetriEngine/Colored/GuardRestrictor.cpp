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

#include "PetriEngine/Colored/GuardRestrictor.h"

namespace PetriEngine::Colored {

GuardRestrictor::GuardRestrictor() = default;

auto GuardRestrictor::get_var_modifier(const std::unordered_map<uint32_t, int32_t> &modPairMap,
                                       uint32_t index) const -> int32_t {
    int32_t modifier;
    for (auto idModPair : modPairMap) {
        if (idModPair.first == index) {
            modifier = idModPair.second;
            break;
        }
    }
    return modifier;
}

auto GuardRestrictor::get_interval_from_ids(const std::vector<uint32_t> &idVec, uint32_t ctSize,
                                            int32_t modifier) const -> interval_t {
    interval_t interval;
    for (auto id : idVec) {
        int32_t val = ctSize + (id + modifier);
        auto colorVal = val % ctSize;
        interval.add_range(colorVal, colorVal);
    }
    return interval;
}

auto GuardRestrictor::get_interval_overlap(const IntervalVector &intervals1,
                                           const IntervalVector &intervals2) const
    -> IntervalVector {
    IntervalVector newIntervalTuple;
    for (const auto &mainInterval : intervals1) {
        for (const auto &otherInterval : intervals2) {
            auto intervalOverlap = otherInterval.get_overlap(mainInterval);

            if (intervalOverlap.is_sound()) {
                newIntervalTuple.add_interval(intervalOverlap);
            }
        }
    }
    return newIntervalTuple;
}

void GuardRestrictor::expand_id_vec(const VariableIntervalMap &varMap,
                                    const VariableModifierMap &mainVarModifierMap,
                                    const VariableModifierMap &otherVarModifierMap,
                                    const PositionVariableMap &varPositions,
                                    const std::unordered_map<uint32_t, const Color *> &constantMap,
                                    const Variable *otherVar, std::vector<uint32_t> &idVec,
                                    size_t targetSize, uint32_t index) const {
    while (idVec.size() < targetSize) {
        if (varPositions.count(index)) {
            const auto &rightTupleInterval = varMap.find(varPositions.find(index)->second)->second;
            int32_t rightVarMod = get_var_modifier(
                mainVarModifierMap.find(varPositions.find(index)->second)->second.back(), index);
            auto ids = rightTupleInterval.get_upper_ids(
                -rightVarMod,
                varPositions.find(index)->second->_colorType->get_constituents_sizes());
            idVec.insert(idVec.end(), ids.begin(), ids.end());
            index += varPositions.find(index)->second->_colorType->product_size();
        } else {
            auto oldSize = idVec.size();
            constantMap.find(index)->second->get_tuple_id(idVec);
            int32_t leftVarMod =
                get_var_modifier(otherVarModifierMap.find(otherVar)->second.back(), index);

            for (auto &id : idVec) {
                id = (otherVar->_colorType->size() + (id + leftVarMod)) %
                     otherVar->_colorType->size();
            }
            index += idVec.size() - oldSize;
        }
    }
}

void GuardRestrictor::expand_interva_vec(
    const VariableIntervalMap &varMap, const VariableModifierMap &mainVarModifierMap,
    const VariableModifierMap &otherVarModifierMap, const PositionVariableMap &varPositions,
    const std::unordered_map<uint32_t, const Color *> &constantMap,
    const Colored::Variable *otherVar, Colored::IntervalVector &intervalVec, size_t targetSize,
    uint32_t index) const {
    while (intervalVec.size() < targetSize) {
        if (varPositions.count(index)) {
            auto rightTupleInterval = varMap.find(varPositions.find(index)->second)->second;
            int32_t rightVarMod = get_var_modifier(
                mainVarModifierMap.find(varPositions.find(index)->second)->second.back(), index);
            rightTupleInterval.apply_modifier(
                -rightVarMod,
                varPositions.find(index)->second->_colorType->get_constituents_sizes());
            intervalVec.append(rightTupleInterval);
            index += varPositions.find(index)->second->_colorType->product_size();
        } else {
            std::vector<uint32_t> colorIdVec;
            constantMap.find(index)->second->get_tuple_id(colorIdVec);
            int32_t leftVarModifier =
                get_var_modifier(otherVarModifierMap.find(otherVar)->second.back(), index);

            for (auto id : colorIdVec) {
                for (auto &interval : intervalVec) {
                    int32_t val = otherVar->_colorType->size() + (id + leftVarModifier);
                    auto colorVal = val % otherVar->_colorType->size();
                    interval.add_range(colorVal, colorVal);
                }
            }
            index += colorIdVec.size();
        }
    }
}

void GuardRestrictor::restrict_by_constant(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &mainVarModifierMap,
    const VariableModifierMap &otherVarModifierMap, const PositionVariableMap &varPositions,
    const std::unordered_map<uint32_t, const Color *> &constantMap, const Colored::Variable *var,
    const Colored::Variable *otherVar, uint32_t index, bool lessthan, bool strict) const {
    for (auto &varMap : variableMap) {
        auto leftColor = constantMap.find(index)->second;
        auto &rightTupleInterval = varMap.find(var)->second;
        std::vector<uint32_t> idVec;
        leftColor->get_tuple_id(idVec);

        expand_id_vec(varMap, mainVarModifierMap, otherVarModifierMap, varPositions, constantMap,
                      otherVar, idVec, rightTupleInterval.tuple_size(), index + idVec.size());

        if (lessthan) {
            rightTupleInterval.constrain_upper(idVec, strict);
        } else {
            rightTupleInterval.constrain_lower(idVec, strict);
        }
    }
}

void GuardRestrictor::restrict_eq_by_constant(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &mainVarModifierMap,
    const VariableModifierMap &otherVarModifierMap, const PositionVariableMap &varPositions,
    const std::unordered_map<uint32_t, const Color *> &constantMap, const Colored::Variable *var,
    uint32_t index) const {
    for (auto &varMap : variableMap) {
        auto color = constantMap.find(index)->second;
        auto &tupleInterval = varMap.find(var)->second;
        std::vector<uint32_t> idVec;
        color->get_tuple_id(idVec);
        int32_t varModifier = get_var_modifier(otherVarModifierMap.find(var)->second.back(), index);

        std::vector<Colored::interval_t> iv{
            get_interval_from_ids(idVec, var->_colorType->size(), varModifier)};
        Colored::IntervalVector intervals(iv);

        expand_interva_vec(varMap, mainVarModifierMap, otherVarModifierMap, varPositions,
                           constantMap, var, intervals, tupleInterval.tuple_size(),
                           index + idVec.size());

        auto newIntervalTupleL = get_interval_overlap(tupleInterval, intervals);
        tupleInterval = newIntervalTupleL;
    }
}

void GuardRestrictor::restrict_diagonal(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &varModifierMapL,
    const VariableModifierMap &varModifierMapR, const PositionVariableMap &varPositionsL,
    const PositionVariableMap &varPositionsR,
    const std::unordered_map<uint32_t, const Color *> &constantMapL,
    const std::unordered_map<uint32_t, const Color *> &constantMapR,
    std::set<const Colored::Variable *> &diagonalVars, const Colored::Variable *var, uint32_t index,
    bool lessthan, bool strict) const {

    diagonalVars.insert(var);
    diagonalVars.insert(varPositionsR.find(index)->second);

    for (auto &varMap : variableMap) {
        if (varMap.count(var) == 0) {
            std::cout << "Unable to find left var " << var->_name << std::endl;
        }
        if (varMap.count(varPositionsR.find(index)->second) == 0) {
            std::cout << "Unable to find right var " << varPositionsR.find(index)->second->_name
                      << std::endl;
        }

        auto &leftTupleInterval = varMap[var];
        auto &rightTupleInterval = varMap[varPositionsR.find(index)->second];
        int32_t leftVarModifier = get_var_modifier(varModifierMapL.find(var)->second.back(), index);
        int32_t rightVarModifier = get_var_modifier(
            varModifierMapR.find(varPositionsR.find(index)->second)->second.back(), index);

        const auto &leftIds = leftTupleInterval.get_lower_ids(
            -leftVarModifier, var->_colorType->get_constituents_sizes());
        const auto &rightIds = rightTupleInterval.get_upper_ids(
            -rightVarModifier,
            varPositionsR.find(index)->second->_colorType->get_constituents_sizes());

        // comparing vars of same size
        if (var->_colorType->product_size() ==
            varPositionsR.find(index)->second->_colorType->product_size()) {
            if (lessthan) {
                leftTupleInterval.constrain_upper(rightIds, strict);
                rightTupleInterval.constrain_lower(leftIds, strict);
            } else {
                leftTupleInterval.constrain_lower(rightIds, strict);
                rightTupleInterval.constrain_upper(leftIds, strict);
            }
        } else if (var->_colorType->product_size() >
                   varPositionsR.find(index)->second->_colorType->product_size()) {
            std::vector<uint32_t> leftLowerVec(leftIds.begin(),
                                               leftIds.begin() + rightTupleInterval.tuple_size());

            auto idVec = rightIds;

            expand_id_vec(varMap, varModifierMapR, varModifierMapL, varPositionsR, constantMapR,
                          var, idVec, leftTupleInterval.tuple_size(),
                          index + varPositionsR.find(index)->second->_colorType->product_size());

            if (lessthan) {
                leftTupleInterval.constrain_upper(idVec, strict);
                rightTupleInterval.constrain_lower(leftLowerVec, strict);
            } else {
                leftTupleInterval.constrain_lower(idVec, strict);
                rightTupleInterval.constrain_upper(leftLowerVec, strict);
            }

        } else {
            std::vector<uint32_t> rightUpperVec(rightIds.begin(),
                                                rightIds.begin() + leftTupleInterval.tuple_size());

            auto idVec = leftIds;
            expand_id_vec(varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL,
                          varPositionsR.find(index)->second, idVec, rightTupleInterval.tuple_size(),
                          index + varPositionsL.find(index)->second->_colorType->product_size());

            if (lessthan) {
                leftTupleInterval.constrain_upper(rightUpperVec, strict);
                rightTupleInterval.constrain_lower(idVec, strict);
            } else {
                leftTupleInterval.constrain_lower(rightUpperVec, strict);
                rightTupleInterval.constrain_upper(idVec, strict);
            }
        }
    }
}

void GuardRestrictor::restrict_eq_diagonal(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &varModifierMapL,
    const VariableModifierMap &varModifierMapR, const PositionVariableMap &varPositionsL,
    const PositionVariableMap &varPositionsR,
    const std::unordered_map<uint32_t, const Color *> &constantMapL,
    const std::unordered_map<uint32_t, const Color *> &constantMapR,
    std::set<const Colored::Variable *> &diagonalVars, const Colored::Variable *var,
    uint32_t index) const {
    diagonalVars.insert(var);
    diagonalVars.insert(varPositionsR.find(index)->second);

    for (auto &varMap : variableMap) {
        auto leftTupleIntervalVal = varMap.find(var)->second;
        auto rightTupleIntervalVal = varMap.find(varPositionsR.find(index)->second)->second;
        auto &leftTupleInterval = varMap.find(var)->second;
        auto &rightTupleInterval = varMap.find(varPositionsR.find(index)->second)->second;
        int32_t leftVarModifier = get_var_modifier(varModifierMapL.find(var)->second.back(), index);
        int32_t rightVarModifier = get_var_modifier(
            varModifierMapR.find(varPositionsR.find(index)->second)->second.back(), index);

        leftTupleIntervalVal.apply_modifier(-leftVarModifier,
                                            var->_colorType->get_constituents_sizes());
        rightTupleIntervalVal.apply_modifier(
            -rightVarModifier,
            varPositionsR.find(index)->second->_colorType->get_constituents_sizes());
        // comparing vars of same size
        if (var->_colorType->product_size() ==
            varPositionsR.find(index)->second->_colorType->product_size()) {
            Colored::IntervalVector newIntervalTuple =
                get_interval_overlap(leftTupleIntervalVal, rightTupleIntervalVal);

            leftTupleInterval = newIntervalTuple;
            rightTupleInterval = newIntervalTuple;
            leftTupleInterval.apply_modifier(leftVarModifier,
                                             var->_colorType->get_constituents_sizes());
            rightTupleInterval.apply_modifier(
                rightVarModifier,
                varPositionsR.find(index)->second->_colorType->get_constituents_sizes());
        } else if (var->_colorType->product_size() >
                   varPositionsR.find(index)->second->_colorType->product_size()) {
            const std::vector<Colored::interval_t> &resizedLeftIntervals =
                leftTupleIntervalVal.shrink_intervals(
                    varPositionsR.find(index)->second->_colorType->product_size());
            auto intervalVec = rightTupleIntervalVal;

            expand_interva_vec(varMap, varModifierMapR, varModifierMapL, varPositionsR,
                               constantMapR, var, intervalVec, leftTupleInterval.tuple_size(),
                               index +
                                   varPositionsR.find(index)->second->_colorType->product_size());

            Colored::IntervalVector newIntervalTupleR =
                get_interval_overlap(rightTupleIntervalVal, resizedLeftIntervals);
            Colored::IntervalVector newIntervalTupleL =
                get_interval_overlap(leftTupleIntervalVal, intervalVec);

            newIntervalTupleL.apply_modifier(leftVarModifier,
                                             var->_colorType->get_constituents_sizes());
            newIntervalTupleR.apply_modifier(
                rightVarModifier,
                varPositionsR.find(index)->second->_colorType->get_constituents_sizes());

            leftTupleInterval = newIntervalTupleL;
            rightTupleInterval = newIntervalTupleR;
        } else {
            std::vector<Colored::interval_t> resizedRightIntervals =
                rightTupleIntervalVal.shrink_intervals(
                    varPositionsL.find(index)->second->_colorType->product_size());
            auto intervalVec = leftTupleIntervalVal;

            expand_interva_vec(
                varMap, varModifierMapR, varModifierMapL, varPositionsL, constantMapL,
                varPositionsR.find(index)->second, intervalVec, rightTupleInterval.tuple_size(),
                index + varPositionsL.find(index)->second->_colorType->product_size());

            Colored::IntervalVector newIntervalTupleL =
                get_interval_overlap(leftTupleIntervalVal, resizedRightIntervals);
            Colored::IntervalVector newIntervalTupleR =
                get_interval_overlap(rightTupleIntervalVal, intervalVec);

            newIntervalTupleL.apply_modifier(leftVarModifier,
                                             var->_colorType->get_constituents_sizes());
            newIntervalTupleR.apply_modifier(
                rightVarModifier,
                varPositionsR.find(index)->second->_colorType->get_constituents_sizes());

            leftTupleInterval = newIntervalTupleL;
            rightTupleInterval = newIntervalTupleR;
        }
    }
}

void GuardRestrictor::restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                                    const VariableModifierMap &varModifierMapL,
                                    const VariableModifierMap &varModifierMapR,
                                    const PositionVariableMap &varPositionsL,
                                    const PositionVariableMap &varPositionsR,
                                    const std::unordered_map<uint32_t, const Color *> &constantMapL,
                                    const std::unordered_map<uint32_t, const Color *> &constantMapR,
                                    std::set<const Colored::Variable *> &diagonalVars,
                                    bool lessthan, bool strict) const {

    for (const auto &varPositionPair : varPositionsL) {
        uint32_t index = varPositionPair.first;
        if (varPositionsR.count(index)) {
            restrict_diagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                              varPositionsR, constantMapL, constantMapR, diagonalVars,
                              varPositionPair.second, index, lessthan, strict);
        } else {
            restrict_by_constant(variableMap, varModifierMapR, varModifierMapL, varPositionsR,
                                 constantMapR, varPositionPair.second, varPositionPair.second,
                                 index, lessthan, strict);
        }
    }

    for (const auto &varPositionPair : varPositionsR) {
        uint32_t index = varPositionPair.first;

        if (constantMapL.count(index)) {
            restrict_by_constant(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                 constantMapL, varPositionPair.second,
                                 varPositionsR.find(index)->second, index, lessthan, strict);
        }
    }
}

void GuardRestrictor::restrict_equality(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &varModifierMapL,
    const VariableModifierMap &varModifierMapR, const PositionVariableMap &varPositionsL,
    const PositionVariableMap &varPositionsR,
    const std::unordered_map<uint32_t, const Color *> &constantMapL,
    const std::unordered_map<uint32_t, const Color *> &constantMapR,
    std::set<const Colored::Variable *> &diagonalVars) const {
    for (const auto &varPositionPair : varPositionsL) {
        uint32_t index = varPositionPair.first;
        if (varPositionsR.count(index)) {
            restrict_eq_diagonal(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                 varPositionsR, constantMapL, constantMapR, diagonalVars,
                                 varPositionPair.second, index);
        } else {
            restrict_eq_by_constant(variableMap, varModifierMapR, varModifierMapL, varPositionsR,
                                    constantMapR, varPositionPair.second, index);
        }
    }

    for (const auto &varPositionPair : varPositionsR) {
        uint32_t index = varPositionPair.first;

        if (constantMapL.count(index)) {
            restrict_eq_by_constant(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                    constantMapL, varPositionPair.second, index);
        }
    }
}

void GuardRestrictor::restrict_inequality(
    std::vector<VariableIntervalMap> &variableMap, const VariableModifierMap &varModifierMapL,
    const VariableModifierMap &varModifierMapR, const PositionVariableMap &varPositionsL,
    const PositionVariableMap &varPositionsR,
    const std::unordered_map<uint32_t, const Color *> &constantMapL,
    const std::unordered_map<uint32_t, const Color *> &constantMapR,
    std::set<const Colored::Variable *> &diagonalVars) const {
    auto variableMapCopy = variableMap;
    restrict_equality(variableMap, varModifierMapL, varModifierMapR, varPositionsL, varPositionsR,
                      constantMapL, constantMapR, diagonalVars);

    for (uint32_t i = 0; i < variableMap.size(); i++) {
        for (const auto &varPosL : varPositionsL) {
            // Check if we are comparing varibles in this position
            if (varPositionsR.find(varPosL.first) != varPositionsR.end()) {
                handle_inequality_vars(variableMapCopy, variableMap, varPosL.second,
                                       varPositionsR.find(varPosL.first)->second, i);
            } else {
                handle_inequalityconstants(variableMapCopy, variableMap, varPosL.second, i);
            }
        }
        for (const auto &varPosR : varPositionsR) {
            if (varPositionsL.find(varPosR.first) == varPositionsL.end()) {
                handle_inequalityconstants(variableMapCopy, variableMap, varPosR.second, i);
            }
        }
    }
}

void GuardRestrictor::handle_inequalityconstants(
    const std::vector<VariableIntervalMap> &variableMapCopy,
    std::vector<VariableIntervalMap> &variableMap, const Variable *var,
    uint32_t varMapIndex) const {
    const uint32_t colorsBefore =
        variableMapCopy[varMapIndex].find(var)->second.get_contained_colors();
    const uint32_t colorsAfter = variableMap[varMapIndex][var].get_contained_colors();
    if (colorsBefore != 0 && colorsAfter == 0) {
        // If the variable did have colors before but none after, then they were not equal
        // and the old colors are kept
        variableMap[varMapIndex][var] = variableMapCopy[varMapIndex].find(var)->second;
    } else if (colorsBefore > 1) {
        // If the variable had multiple colors and colors we invert the colors after applying
        // equivalence
        invert_intervals(variableMap[varMapIndex][var],
                         variableMapCopy[varMapIndex].find(var)->second, var->_colorType);
    } else if (colorsBefore == 1) {
        // If there was only one color and we still have a color after, they were equal
        // and the empty interval is set
        variableMap[varMapIndex][var].clear();
    }
}

void GuardRestrictor::handle_inequality_vars(
    const std::vector<VariableIntervalMap> &variableMapCopy,
    std::vector<VariableIntervalMap> &variableMap, const Variable *var1, const Variable *var2,
    uint32_t varMapIndex) const {
    const uint32_t rightColorsBefore =
        variableMapCopy[varMapIndex].find(var2)->second.get_contained_colors();
    const uint32_t rightColorsAfter = variableMap[varMapIndex][var2].get_contained_colors();
    const uint32_t leftColorsBefore =
        variableMapCopy[varMapIndex].find(var1)->second.get_contained_colors();
    const uint32_t leftColorsAfter = variableMap[varMapIndex][var1].get_contained_colors();
    if ((leftColorsBefore > 1 && rightColorsBefore > 1) || leftColorsBefore == 0 ||
        rightColorsBefore == 0) {
        // If both variables have more than one color in their interval we cannot restrict the
        // intervals and if either side did not have any colors the varmap is not valid.
        return;
    } else if (leftColorsBefore == 1 && rightColorsBefore == 1) {
        // if we are comparing single colors and there is an interval after equality
        // it means the colors were equal so no interval can be kept for inequality.
        // Otherwise they were not equal and their intervals can be kept
        if (!variableMap[varMapIndex][var1].empty()) {
            variableMap[varMapIndex][var1].clear();
            variableMap[varMapIndex][var2].clear();
        } else {
            variableMap[varMapIndex][var1] = variableMapCopy[varMapIndex].find(var1)->second;
            variableMap[varMapIndex][var2] = variableMapCopy[varMapIndex].find(var2)->second;
        }
    } else if (leftColorsBefore > 1) {
        if (leftColorsAfter > 0) {
            // If one side has more colors but the other does not, we invert that side
            invert_intervals(variableMap[varMapIndex][var1],
                             variableMapCopy[varMapIndex].find(var1)->second, var1->_colorType);
        } else {
            variableMap[varMapIndex][var1] = variableMapCopy[varMapIndex].find(var1)->second;
            variableMap[varMapIndex][var2] = variableMapCopy[varMapIndex].find(var2)->second;
        }
    } else {
        if (rightColorsAfter > 0) {
            // If one side has more colors but the other does not, we invert that side
            invert_intervals(variableMap[varMapIndex][var2],
                             variableMapCopy[varMapIndex].find(var2)->second, var2->_colorType);
        } else {
            variableMap[varMapIndex][var1] = variableMapCopy[varMapIndex].find(var1)->second;
            variableMap[varMapIndex][var2] = variableMapCopy[varMapIndex].find(var2)->second;
        }
    }
}

// Retrieve the intervals remaining when the intervals created from restricting by equvalence
// are removed from the original intervals
void GuardRestrictor::invert_intervals(IntervalVector &intervals,
                                       const IntervalVector &oldIntervals,
                                       const ColorType *colorType) const {
    const std::vector<bool> diagonalPositions(colorType->size(), false);
    IntervalVector invertedIntervalvec;

    for (const auto &oldInterval : oldIntervals) {
        std::vector<PetriEngine::Colored::interval_t> subtractionRes;
        for (const auto &interval : intervals) {
            if (subtractionRes.empty()) {
                subtractionRes = oldInterval.get_substracted(interval, diagonalPositions);
            } else {
                std::vector<PetriEngine::Colored::interval_t> tempSubtractionRes;
                const auto &vecIntervals = oldInterval.get_substracted(interval, diagonalPositions);
                for (const auto &curInterval : subtractionRes) {
                    for (const auto &newInterval : vecIntervals) {
                        const auto &overlappingInterval = curInterval.get_overlap(newInterval);
                        if (overlappingInterval.is_sound()) {
                            tempSubtractionRes.push_back(overlappingInterval);
                        }
                    }
                }
                subtractionRes = std::move(tempSubtractionRes);
            }
        }
        for (const auto &interval : subtractionRes) {
            invertedIntervalvec.add_interval(interval);
        }
    }

    intervals = invertedIntervalvec;
}

auto GuardRestrictor::shift_intervals(const VariableIntervalMap &varMap,
                                      const std::vector<const Colored::ColorType *> &colortypes,
                                      PetriEngine::Colored::IntervalVector &intervals,
                                      int32_t modifier, uint32_t ctSizeBefore) const
    -> IntervalVector {
    Colored::IntervalVector newIntervals;
    for (auto &interval : intervals) {
        Colored::interval_t newInterval;
        std::vector<Colored::interval_t> tempIntervals;
        for (uint32_t j = 0; j < interval._ranges.size(); j++) {
            auto &range = interval.operator[](j);
            size_t ctSize = colortypes[j + ctSizeBefore]->size();

            auto shiftedInterval =
                newIntervals.shift_interval(range._lower, range._upper, ctSize, modifier);
            range._lower = shiftedInterval.first;
            range._upper = shiftedInterval.second;

            if (range._upper + 1 == range._lower) {
                if (tempIntervals.empty()) {
                    newInterval.add_range(0, ctSize - 1);
                    tempIntervals.push_back(newInterval);
                } else {
                    for (auto &tempInterval : tempIntervals) {
                        tempInterval.add_range(0, ctSize - 1);
                    }
                }
            } else if (range._upper < range._lower) {

                if (tempIntervals.empty()) {
                    auto intervalCopy = newInterval;
                    newInterval.add_range(range._lower, ctSize - 1);
                    intervalCopy.add_range(0, range._upper);
                    tempIntervals.push_back(newInterval);
                    tempIntervals.push_back(intervalCopy);
                } else {
                    std::vector<Colored::interval_t> newTempIntervals;
                    for (auto tempInterval : tempIntervals) {
                        auto intervalCopy = tempInterval;
                        tempInterval.add_range(range._lower, ctSize - 1);
                        intervalCopy.add_range(0, range._upper);
                        newTempIntervals.push_back(intervalCopy);
                        newTempIntervals.push_back(tempInterval);
                    }
                    tempIntervals = newTempIntervals;
                }
            } else {
                if (tempIntervals.empty()) {
                    newInterval.add_range(range);
                    tempIntervals.push_back(newInterval);
                } else {
                    for (auto &tempInterval : tempIntervals) {
                        tempInterval.add_range(range);
                    }
                }
            }
        }
        for (const auto &tempInterval : tempIntervals) {
            newIntervals.add_interval(tempInterval);
        }
    }
    return newIntervals;
}
} // namespace PetriEngine::Colored
