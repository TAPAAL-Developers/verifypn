/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef PETRINET_H
#define PETRINET_H

#include <climits>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace PetriEngine {

namespace PQL {
class Condition;
}

namespace Structures {
class State;
}

class PetriNetBuilder;
class SuccessorGenerator;

struct trans_ptr_t {
    uint32_t _inputs;
    uint32_t _outputs;
};

struct invariant_t {
    uint32_t _place;
    uint32_t _tokens;
    bool _inhibitor;
    int8_t _direction;
    // we can pack things here, but might give slowdown
} /*__attribute__((packed))*/;

/** Type used for holding markings values */
using MarkVal = uint32_t;

/** Efficient representation of PetriNet */
class PetriNet {
    PetriNet(uint32_t transitions, uint32_t invariants, uint32_t places);

  public:
    ~PetriNet();

    PetriNet(PetriNet &&) = delete;

    [[nodiscard]] auto initial(size_t id) const -> uint32_t;
    [[nodiscard]] auto make_initial_marking() const -> MarkVal *;
    /** Fire transition if possible and store result in result */
    auto deadlocked(const MarkVal *marking) const -> bool;
    auto fireable(const MarkVal *marking, int transitionIndex) -> bool;
    [[nodiscard]] auto preset(uint32_t id) const
        -> std::pair<const invariant_t *, const invariant_t *>;
    [[nodiscard]] auto postset(uint32_t id) const
        -> std::pair<const invariant_t *, const invariant_t *>;
    [[nodiscard]] auto number_of_transitions() const -> uint32_t { return _ntransitions; }

    [[nodiscard]] auto number_of_places() const -> uint32_t { return _nplaces; }
    [[nodiscard]] auto in_arc(uint32_t place, uint32_t transition) const -> uint32_t;
    [[nodiscard]] auto out_arc(uint32_t transition, uint32_t place) const -> uint32_t;

    [[nodiscard]] auto transition_names() const -> const std::vector<std::string> & {
        return _transitionnames;
    }

    [[nodiscard]] auto place_names() const -> const std::vector<std::string> & {
        return _placenames;
    }

    void print(MarkVal const *const val) const {
        for (size_t i = 0; i < _nplaces; ++i) {
            if (val[i] != 0) {
                std::cout << _placenames[i] << "(" << i << ")"
                          << " -> " << val[i] << std::endl;
            }
        }
    }

    void sort();

    void to_xml(std::ostream &out);

    [[nodiscard]] auto initial() const -> const MarkVal * { return _initialMarking; }

    [[nodiscard]] auto has_inhibitor() const -> bool {
        for (invariant_t i : _invariants) {
            if (i._inhibitor)
                return true;
        }
        return false;
    }

  private:
    /** Number of x variables
     * @remarks We could also get this from the _places vector, but I don't see any
     * any complexity garentees for this type.
     */
    uint32_t _ninvariants, _ntransitions, _nplaces;

    std::vector<trans_ptr_t> _transitions;
    std::vector<invariant_t> _invariants;
    std::vector<uint32_t> _placeToPtrs;
    MarkVal *_initialMarking;

    std::vector<std::string> _transitionnames;
    std::vector<std::string> _placenames;

    std::vector<std::tuple<double, double>> _placelocations;
    std::vector<std::tuple<double, double>> _transitionlocations;

    friend class PetriNetBuilder;
    friend class Reducer;
    friend class SuccessorGenerator;
    friend class ReducingSuccessorGenerator;
    friend class STSolver;
    friend class StubbornSet;
};

} // namespace PetriEngine

#endif // PETRINET_H
