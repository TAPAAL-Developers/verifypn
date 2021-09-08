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

#include <PetriEngine/Stubborn/InterestingTransitionVisitor.h>

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Stubborn/StubbornSet.h"
#include <memory>

namespace PetriEngine {
auto StubbornSet::next() -> uint32_t {
    while (!_ordering.empty()) {
        _current = _ordering.front();
        _ordering.pop_front();
        if (_stubborn[_current] && _enabled[_current]) {
            return _current;
        }
    }
    return std::numeric_limits<uint32_t>::max();
}

auto StubbornSet::check_preset(uint32_t t) -> bool {
    const TransPtr &ptr = transitions()[t];
    uint32_t finv = ptr._inputs;
    uint32_t linv = ptr._outputs;

    for (; finv < linv; ++finv) {
        const Invariant &inv = _net._invariants[finv];
        if (_parent->marking()[inv._place] < inv._tokens) {
            if (!inv._inhibitor) {
                return false;
            }
        } else {
            if (inv._inhibitor) {
                return false;
            }
        }
    }
    return true;
}

auto StubbornSet::seen_pre(uint32_t place) const -> bool {
    return (_places_seen.get()[place] & _presetSeen) != 0;
}

auto StubbornSet::seen_post(uint32_t place) const -> bool {
    return (_places_seen.get()[place] & _postsetSeen) != 0;
}

void StubbornSet::preset_of(uint32_t place, bool make_closure) {
    if ((_places_seen[place] & _presetSeen) != 0)
        return;
    _places_seen[place] = _places_seen[place] | _presetSeen;
    for (uint32_t t = _places[place]._pre; t < _places[place]._post; t++) {
        auto &tr = _transitions[t];
        add_to_stub(tr._index);
    }
    if (make_closure)
        closure();
}

void StubbornSet::postset_of(uint32_t place, bool make_closure) {
    if ((_places_seen[place] & _postsetSeen) != 0)
        return;
    _places_seen[place] = _places_seen[place] | _postsetSeen;
    for (uint32_t t = _places[place]._post; t < _places[place + 1]._pre; t++) {
        auto tr = _transitions[t];
        if (tr._direction < 0)
            add_to_stub(tr._index);
    }
    if (make_closure)
        closure();
}

void StubbornSet::inhibitor_postset_of(uint32_t place) {
    if ((_places_seen[place] & _inhibPostsetSeen) != 0)
        return;
    _places_seen[place] = _places_seen[place] | _inhibPostsetSeen;
    for (uint32_t &newstub : _inhibpost[place])
        add_to_stub(newstub);
}

void StubbornSet::post_preset_of(uint32_t t, bool make_closure) {
    const TransPtr &ptr = transitions()[t];
    uint32_t finv = ptr._inputs;
    uint32_t linv = ptr._outputs;
    for (; finv < linv; finv++) { // pre-set of t
        if (invariants()[finv]._inhibitor) {
            preset_of(invariants()[finv]._place, make_closure);
        } else {
            postset_of(invariants()[finv]._place, make_closure);
        }
    }
}

void StubbornSet::construct_enabled() {
    _ordering.clear();
    memset(_enabled.get(), 0, _net.number_of_transitions());
    memset(_stubborn.get(), 0, _net.number_of_transitions());
    for (uint32_t p = 0; p < _net.number_of_places(); ++p) {
        // orphans are currently under "place 0" as a special case
        if (p == 0 || _parent->marking()[p] > 0) {
            uint32_t t = place_to_ptrs()[p];
            uint32_t last = place_to_ptrs()[p + 1];

            for (; t != last; ++t) {
                if (!check_preset(t)) {
                    continue;
                }
                _enabled[t] = true;
                _ordering.push_back(t);
                ++_nenabled;
            }
        }
    }
}

void StubbornSet::check_for_inhibitor() {
    _netContainsInhibitorArcs = false;
    for (uint32_t t = 0; t < _net._ntransitions; t++) {
        const TransPtr &ptr = _net._transitions[t];
        uint32_t finv = ptr._inputs;
        uint32_t linv = ptr._outputs;
        for (; finv < linv; finv++) { // Post set of places
            if (_net._invariants[finv]._inhibitor) {
                _netContainsInhibitorArcs = true;
                return;
            }
        }
    }
}

void StubbornSet::construct_pre_post() {
    std::vector<std::pair<std::vector<trans_t>, std::vector<trans_t>>> tmp_places(_net._nplaces);

    for (uint32_t t = 0; t < _net._ntransitions; t++) {
        const TransPtr &ptr = _net._transitions[t];
        uint32_t finv = ptr._inputs;
        uint32_t linv = ptr._outputs;
        for (; finv < linv; finv++) { // Post set of places
            if (_net._invariants[finv]._inhibitor) {
                _inhibpost[_net._invariants[finv]._place].push_back(t);
                _netContainsInhibitorArcs = true;
            } else {
                tmp_places[_net._invariants[finv]._place].second.emplace_back(
                    t, _net._invariants[finv]._direction);
            }
        }

        finv = linv;
        linv = _net._transitions[t + 1]._inputs;
        for (; finv < linv; finv++) { // Pre set of places
            if (_net._invariants[finv]._direction > 0)
                tmp_places[_net._invariants[finv]._place].first.emplace_back(
                    t, _net._invariants[finv]._direction);
        }
    }

    // flatten
    size_t ntrans = 0;
    for (const auto &p : tmp_places) {
        ntrans += p.first.size() + p.second.size();
    }
    _transitions = std::make_unique<trans_t[]>(ntrans);

    _places = std::make_unique<place_t[]>(_net._nplaces + 1);
    uint32_t offset = 0;
    uint32_t p = 0;
    for (; p < _net._nplaces; ++p) {
        auto &pre = tmp_places[p].first;
        auto &post = tmp_places[p].second;

        // keep things nice for caches
        std::sort(pre.begin(), pre.end());
        std::sort(post.begin(), post.end());

        _places[p]._pre = offset;
        offset += pre.size();
        _places[p]._post = offset;
        offset += post.size();
        for (size_t tn = 0; tn < pre.size(); ++tn) {
            _transitions[tn + _places[p]._pre] = pre[tn];
        }

        for (size_t tn = 0; tn < post.size(); ++tn) {
            _transitions[tn + _places[p]._post] = post[tn];
        }
    }
    assert(offset == ntrans);
    _places[p]._pre = offset;
    _places[p]._post = offset;
}

void StubbornSet::construct_dependency() {
    memset(_dependency.get(), 0, sizeof(uint32_t) * _net._ntransitions);

    for (uint32_t t = 0; t < _net._ntransitions; t++) {
        uint32_t finv = _net._transitions[t]._inputs;
        uint32_t linv = _net._transitions[t]._outputs;

        for (; finv < linv; finv++) {
            const Invariant &inv = _net._invariants[finv];
            uint32_t p = inv._place;
            uint32_t ntrans = (_places[p + 1]._pre - _places[p]._post);

            for (uint32_t tIndex = 0; tIndex < ntrans; tIndex++) {
                ++_dependency[t];
            }
        }
    }
}

void StubbornSet::add_to_stub(uint32_t t) {
    if (!_stubborn[t]) {
        _stubborn[t] = true;
        _unprocessed.push_back(t);
    }
}

auto StubbornSet::least_dependent_enabled() -> uint32_t {
    uint32_t tLeast = -1;
    bool foundLeast = false;
    for (uint32_t t = 0; t < _net.number_of_transitions(); t++) {
        if (_enabled[t]) {
            if (!foundLeast) {
                tLeast = t;
                foundLeast = true;
            } else {
                if (_dependency[t] < _dependency[tLeast]) {
                    tLeast = t;
                }
            }
        }
    }
    return tLeast;
}

void StubbornSet::reset() {
    memset(_enabled.get(), false, sizeof(bool) * _net.number_of_transitions());
    memset(_stubborn.get(), false, sizeof(bool) * _net.number_of_transitions());
    _ordering.clear();
    _nenabled = 0;
    //_tid = 0;
    _done = false;
}
} // namespace PetriEngine
