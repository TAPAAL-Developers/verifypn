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

#include <unordered_map>
#include <vector>

#include "ColoredNetStructures.h"
#include "EquivalenceClass.h"

namespace PetriEngine {

class NaiveBindingGenerator {
  public:
    class Iterator {
      private:
        NaiveBindingGenerator *_generator;

      public:
        Iterator(NaiveBindingGenerator *generator);

        auto operator==(Iterator &other) -> bool;
        auto operator!=(Iterator &other) -> bool;
        auto operator++() -> Iterator &;
        auto operator*() const -> const Colored::BindingMap &;
    };

  private:
    Colored::GuardExpression_ptr _expr;
    Colored::BindingMap _bindings;
    Colored::ColorTypeMap &_colorTypes;

    auto eval() const -> bool;

  public:
    NaiveBindingGenerator(const Colored::transition_t &transition,
                          Colored::ColorTypeMap &colorTypes);

    auto next_binding() -> const Colored::BindingMap &;
    auto current_binding() const -> const Colored::BindingMap &;
    auto is_initial() const -> bool;
    auto begin() -> Iterator;
    auto end() -> Iterator;
};

class FixpointBindingGenerator {
  public:
    class Iterator {
      private:
        FixpointBindingGenerator *_generator;

      public:
        Iterator(FixpointBindingGenerator *generator);

        auto operator==(Iterator &other) -> bool;
        auto operator!=(Iterator &other) -> bool;
        auto operator++() -> Iterator &;
        auto operator++(int) -> const Colored::BindingMap;
        auto operator*() const -> const Colored::BindingMap &;
    };

  private:
    const Colored::GuardExpression_ptr &_expr;
    Colored::BindingMap _bindings;
    std::vector<std::vector<std::vector<uint32_t>>> _symmetric_var_combinations;
    const Colored::ColorTypeMap &_colorTypes;
    const Colored::transition_t &_transition;
    const std::vector<std::set<const Colored::Variable *>> &_symmetric_vars;
    Colored::BindingMap::iterator _bindingIterator;
    bool _isDone;
    bool _noValidBindings;
    uint32_t _nextIndex = 0;
    uint32_t _currentOuterId = 0;
    uint32_t _currentInnerId = 0;
    uint32_t _symmetric_vars_set = 0;

    auto eval() const -> bool;
    auto assign_symmetric_vars() -> bool;
    void generate_combinations(uint32_t options, uint32_t samples,
                               std::vector<std::vector<uint32_t>> &result,
                               std::vector<uint32_t> &current) const;

  public:
    FixpointBindingGenerator(
        const Colored::transition_t &transition, const Colored::ColorTypeMap &colorTypes,
        const std::vector<std::set<const Colored::Variable *>> &symmetric_vars);

    FixpointBindingGenerator(const FixpointBindingGenerator &) = default;

    auto operator=(const FixpointBindingGenerator &b) -> FixpointBindingGenerator & = default;

    auto next_binding() -> const Colored::BindingMap &;
    auto current_binding() const -> const Colored::BindingMap &;
    auto is_initial() const -> bool;
    auto begin() -> Iterator;
    auto end() -> Iterator;
};
} // namespace PetriEngine