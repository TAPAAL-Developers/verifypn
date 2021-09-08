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

#ifndef VERIFYPN_VISIBLELTLSTUBBORNSET_H
#define VERIFYPN_VISIBLELTLSTUBBORNSET_H

//#include "PetriEngine/ReducingSuccessorGenerator.h"
#include "LTL/Stubborn/VisibleTransitionVisitor.h"
#include "LTL/SuccessorGeneration/SuccessorSpooler.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/Stubborn/StubbornSet.h"

// TODO LTL Stubborn sets should be subclassing just PetriEngine::StubbornSet, then a class
// LTL::StubbornSuccessorSpooler : LTL::SuccessorSpooler has a stubborn set via simple wrapping.
// This way we can avoid the super ugly multiple inheritance caused by both base classes
// each having prepare, next, reset methods.q
namespace LTL {
class VisibleLTLStubbornSet : public PetriEngine::StubbornSet, public SuccessorSpooler {
  public:
    VisibleLTLStubbornSet(const PetriEngine::PetriNet &net,
                          const std::vector<PetriEngine::PQL::Condition_ptr> &queries)
        : StubbornSet(net, queries), _visible(new bool[net.number_of_transitions()]) {
        assert(!_netContainsInhibitorArcs);
        memset(_visible.get(), 0, sizeof(bool) * net.number_of_places());
        VisibleTransitionVisitor visible{_visible};
        for (auto &q : queries) {
            q->visit(visible);
        }
    }

    VisibleLTLStubbornSet(const PetriEngine::PetriNet &net,
                          const PetriEngine::PQL::Condition_ptr &query)
        : StubbornSet(net, query), _visible(new bool[net.number_of_transitions()]) {
        assert(!_netContainsInhibitorArcs);
        auto places = std::make_unique<bool[]>(net.number_of_places());
        memset(places.get(), 0, sizeof(bool) * net.number_of_places());
        memset(_visible.get(), 0, sizeof(bool) * net.number_of_transitions());
        VisibleTransitionVisitor visible{places};
        query->visit(visible);

        memset(_places_seen.get(), 0, _net.number_of_places());
        for (uint32_t p = 0; p < net.number_of_places(); ++p) {
            if (places[p]) {
                set_transition_visible(p);
            }
        }
    }

    void set_transition_visible(uint32_t place) {
        if (_places_seen[place] > 0)
            return;
        _places_seen[place] = 1;
        for (uint32_t t = _places[place]._pre; t < _places[place]._post; ++t) {
            auto tr = _transitions[t];
            _visible[tr._index] = true;
        }
        for (uint32_t t = _places[place]._post; t < _places[place + 1]._pre; t++) {
            auto tr = _transitions[t];
            if (tr._direction < 0)
                _visible[tr._index] = true;
        }
    }

    auto prepare(const PetriEngine::Structures::State &marking) -> bool override;

    auto prepare(const LTL::Structures::ProductState &marking) -> bool override {
        return prepare((const PetriEngine::Structures::State &)marking);
    }

    auto next() -> uint32_t override;

    void reset() override;

    auto generate_all(const LTL::Structures::ProductState &parent) -> bool override;

  protected:
    void add_to_stub(uint32_t t) override;

  private:
    std::unique_ptr<bool[]> _visible;
    LightDequeue<uint32_t> _skipped;
    bool _has_enabled_stubborn;

    void find_key_transition();

    void ensure_rule_v();

    void ensure_rules_l();
};
} // namespace LTL

#endif // VERIFYPN_VISIBLELTLSTUBBORNSET_H
