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

#ifndef GuardRestrictions_H
#define GuardRestrictions_H

#include "Colors.h"
#include "Multiset.h"
#include <cstdlib>
#include <set>
#include <unordered_map>

namespace PetriEngine::Colored {

class GuardRestrictor {
  public:
    GuardRestrictor();

    void restrict_diagonal(std::vector<VariableIntervalMap> &variableMap,
                           const VariableModifierMap &varModifierMapL,
                           const VariableModifierMap &varModifierMapR,
                           const PositionVariableMap &varPositionsL,
                           const PositionVariableMap &varPositionsR,
                           const std::unordered_map<uint32_t, const Color *> &constantMapL,
                           const std::unordered_map<uint32_t, const Color *> &constantMapR,
                           std::set<const Colored::Variable *> &diagonalVars,
                           const Colored::Variable *var, uint32_t index, bool lessthan,
                           bool strict) const;

    void restrict_equality(std::vector<VariableIntervalMap> &variableMap,
                           const VariableModifierMap &varModifierMapL,
                           const VariableModifierMap &varModifierMapR,
                           const PositionVariableMap &varPositionsL,
                           const PositionVariableMap &varPositionsR,
                           const std::unordered_map<uint32_t, const Color *> &constantMapL,
                           const std::unordered_map<uint32_t, const Color *> &constantMapR,
                           std::set<const Colored::Variable *> &diagonalVars) const;

    void restrict_inequality(std::vector<VariableIntervalMap> &variableMap,
                             const VariableModifierMap &varModifierMapL,
                             const VariableModifierMap &varModifierMapR,
                             const PositionVariableMap &varPositionsL,
                             const PositionVariableMap &varPositionsR,
                             const std::unordered_map<uint32_t, const Color *> &constantMapL,
                             const std::unordered_map<uint32_t, const Color *> &constantMapR,
                             std::set<const Colored::Variable *> &diagonalVars) const;

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       const VariableModifierMap &varModifierMapL,
                       const VariableModifierMap &varModifierMapR,
                       const PositionVariableMap &varPositionsL,
                       const PositionVariableMap &varPositionsR,
                       const std::unordered_map<uint32_t, const Color *> &constantMapL,
                       const std::unordered_map<uint32_t, const Color *> &constantMapR,
                       std::set<const Colored::Variable *> &diagonalVars, bool lessthan,
                       bool strict) const;

    auto shift_intervals(const VariableIntervalMap &varMap,
                         const std::vector<const ColorType *> &colortypes,
                         IntervalVector &intervals, int32_t modifier,
                         uint32_t ctSizeBefore) const -> IntervalVector;

  private:
    [[nodiscard]] auto get_var_modifier(const std::unordered_map<uint32_t, int32_t> &modPairMap,
                                        uint32_t index) const -> int32_t;
    [[nodiscard]] auto get_interval_from_ids(const std::vector<uint32_t> &idVec, uint32_t ctSize,
                                             int32_t modifier) const -> interval_t;
    [[nodiscard]] auto get_interval_overlap(const Colored::IntervalVector &intervals1,
                                            const Colored::IntervalVector &intervals2) const
        -> IntervalVector;
    void invert_intervals(IntervalVector &intervals, const IntervalVector &oldIntervals,
                          const ColorType *colorType) const;

    void handle_inequalityconstants(const std::vector<VariableIntervalMap> &variableMapCopy,
                                    std::vector<VariableIntervalMap> &variableMap,
                                    const Variable *var, uint32_t varMapIndex) const;

    void handle_inequality_vars(const std::vector<VariableIntervalMap> &variableMapCopy,
                                std::vector<VariableIntervalMap> &variableMap, const Variable *var1,
                                const Variable *var2, uint32_t varMapIndex) const;

    void expand_id_vec(const VariableIntervalMap &varMap,
                       const VariableModifierMap &mainVarModifierMap,
                       const VariableModifierMap &otherVarModifierMap,
                       const std::unordered_map<uint32_t, const Variable *> &varPositions,
                       const std::unordered_map<uint32_t, const Color *> &constantMap,
                       const Variable *otherVar, std::vector<uint32_t> &idVec, size_t targetSize,
                       uint32_t index) const;

    void expand_interva_vec(const VariableIntervalMap &varMap,
                            const VariableModifierMap &mainVarModifierMap,
                            const VariableModifierMap &otherVarModifierMap,
                            const std::unordered_map<uint32_t, const Variable *> &varPositions,
                            const std::unordered_map<uint32_t, const Color *> &constantMap,
                            const Variable *otherVar, IntervalVector &intervalVec,
                            size_t targetSize, uint32_t index) const;

    void restrict_by_constant(std::vector<VariableIntervalMap> &variableMap,
                              const VariableModifierMap &mainVarModifierMap,
                              const VariableModifierMap &otherVarModifierMap,
                              const PositionVariableMap &varPositions,
                              const std::unordered_map<uint32_t, const Color *> &constantMap,
                              const Colored::Variable *var, const Colored::Variable *otherVar,
                              uint32_t index, bool lessthan, bool strict) const;

    void restrict_eq_by_constant(std::vector<VariableIntervalMap> &variableMap,
                                 const VariableModifierMap &mainVarModifierMap,
                                 const VariableModifierMap &otherVarModifierMap,
                                 const PositionVariableMap &varPositions,
                                 const std::unordered_map<uint32_t, const Color *> &constantMap,
                                 const Colored::Variable *var, uint32_t index) const;

    void restrict_eq_diagonal(std::vector<VariableIntervalMap> &variableMap,
                              const VariableModifierMap &varModifierMapL,
                              const VariableModifierMap &varModifierMapR,
                              const PositionVariableMap &varPositionsL,
                              const PositionVariableMap &varPositionsR,
                              const std::unordered_map<uint32_t, const Color *> &constantMapL,
                              const std::unordered_map<uint32_t, const Color *> &constantMapR,
                              std::set<const Colored::Variable *> &diagonalVars,
                              const Colored::Variable *var, uint32_t index) const;
};
} // namespace PetriEngine::Colored

#endif /* GuardRestrictions_H */