/* Copyright (C) 2020  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#ifndef VERIFYPN_PRODUCTSTATE_H
#define VERIFYPN_PRODUCTSTATE_H

#include "LTL/Structures/BuchiAutomaton.h"
#include "PetriEngine/Structures/State.h"

namespace LTL {
template <class SuccessorGen> class ProductSuccessorGenerator;
}
namespace LTL::Structures {
/**
 * State on the form (M, q) for marking M and NBA state q.
 * Represented as array of size nplaces + 1, where the last element is the number
 * of NBA state q, and the first nplaces elements are the actual marking.
 */
class ProductState : public PetriEngine::Structures::State {
  public:
    explicit ProductState(const BuchiAutomaton *aut = nullptr)
        : PetriEngine::Structures::State(), _aut(aut) {}

    void set_marking(PetriEngine::MarkVal *marking, size_t nplaces) {
        State::set_marking(marking);
        // because zero-indexing
        _buchi_state_idx = nplaces;
    }

    [[nodiscard]] auto get_buchi_state() const -> uint32_t { return marking()[_buchi_state_idx]; }

    void set_buchi_state(uint32_t state) { marking()[_buchi_state_idx] = state; }

    [[nodiscard]] auto marking_equal(const PetriEngine::MarkVal *rhs) const -> bool {
        for (size_t i = 0; i < _buchi_state_idx; ++i) {
            if (marking()[i] != rhs[i]) {
                return false;
            }
        }
        return true;
    }

    auto operator==(const ProductState &rhs) const -> bool {
        for (size_t i = 0; i <= _buchi_state_idx; ++i) {
            if (marking()[i] != rhs.marking()[i]) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] auto size() const -> size_t { return _buchi_state_idx + 1; }

    auto operator!=(const ProductState &rhs) const -> bool { return !(rhs == *this); }

    [[nodiscard]] auto is_accepting() const -> bool {
        if (_aut)
            return _aut->_buchi->state_is_accepting(marking()[_buchi_state_idx]);
        assert(false);
        return false;
    }

  private:
    template <typename T> friend class LTL::ProductSuccessorGenerator;
    size_t _buchi_state_idx;
    const LTL::Structures::BuchiAutomaton *_aut = nullptr;
};
} // namespace LTL::Structures

#endif // VERIFYPN_PRODUCTSTATE_H
