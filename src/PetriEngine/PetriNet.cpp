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
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/Structures/State.h"

#include <cassert>
#include <cstring>

namespace PetriEngine {

PetriNet::PetriNet(uint32_t trans, uint32_t invariants, uint32_t places)
    : _ninvariants(invariants), _ntransitions(trans), _nplaces(places),
      _transitions(_ntransitions + 1), _invariants(_ninvariants), _placeToPtrs(_nplaces + 1) {

    // to avoid special cases
    _transitions[_ntransitions]._inputs = _ninvariants;
    _transitions[_ntransitions]._outputs = _ninvariants;
    _placeToPtrs[_nplaces] = _ntransitions;
    _initialMarking = new MarkVal[_nplaces];
    //        assert(_nplaces > 0);
}

PetriNet::~PetriNet() { delete[] _initialMarking; }

auto PetriNet::in_arc(uint32_t place, uint32_t transition) const -> uint32_t {
    assert(_nplaces > 0);
    assert(place < _nplaces);
    assert(transition < _ntransitions);

    uint32_t imin = _transitions[transition]._inputs;
    uint32_t imax = _transitions[transition]._outputs;
    if (imin == imax) {
        // NO INPUT!
        return 0;
    }

    for (; imin < imax; ++imin) {
        const invariant_t &inv = _invariants[imin];
        if (inv._place == place) {
            return inv._inhibitor ? 0 : _invariants[imin]._tokens;
        }
    }
    return 0;
}
auto PetriNet::out_arc(uint32_t transition, uint32_t place) const -> uint32_t {
    assert(_nplaces > 0);
    assert(place < _nplaces);
    assert(transition < _ntransitions);

    uint32_t imin = _transitions[transition]._outputs;
    uint32_t imax = _transitions[transition + 1]._inputs;
    for (; imin < imax; ++imin) {
        if (_invariants[imin]._place == place)
            return _invariants[imin]._tokens;
    }
    return 0;
}

auto PetriNet::deadlocked(const MarkVal *m) const -> bool {
    // Check that we can take from the marking
    if (_nplaces == 0) {
        return _ntransitions == 0;
    }
    for (size_t i = 0; i < _nplaces; i++) {
        if (i == 0 || m[i] > 0) // orphans are currently under "place 0" as a special case
        {
            uint32_t first = _placeToPtrs[i];
            uint32_t last = _placeToPtrs[i + 1];
            for (; first != last; ++first) {
                const trans_ptr_t &ptr = _transitions[first];
                uint32_t finv = ptr._inputs;
                uint32_t linv = ptr._outputs;
                bool allgood = true;
                for (; finv != linv; ++finv) {
                    if (!_invariants[finv]._inhibitor) {
                        allgood &= m[_invariants[finv]._place] >= _invariants[finv]._tokens;
                    } else {
                        allgood &= m[_invariants[finv]._place] < _invariants[finv]._tokens;
                    }
                    if (!allgood) {
                        break;
                    }
                }

                if (allgood) {
                    return false;
                }
            }
        }
    }
    return true;
}

auto PetriNet::preset(uint32_t id) const -> std::pair<const invariant_t *, const invariant_t *> {
    const trans_ptr_t &transition = _transitions[id];
    uint32_t first = transition._inputs;
    uint32_t last = transition._outputs;
    return std::make_pair(&_invariants[first], &_invariants[last]);
}

auto PetriNet::postset(uint32_t id) const -> std::pair<const invariant_t *, const invariant_t *> {
    uint32_t first = _transitions[id]._outputs;
    uint32_t last = _transitions[id + 1]._inputs;
    return std::make_pair(&_invariants[first], &_invariants[last]);
}

auto PetriNet::fireable(const MarkVal *marking, int transitionIndex) -> bool {
    const trans_ptr_t &transition = _transitions[transitionIndex];
    uint32_t first = transition._inputs;
    uint32_t last = transition._outputs;

    for (; first < last; ++first) {
        const invariant_t &inv = _invariants[first];
        if (inv._inhibitor == (marking[inv._place] >= inv._tokens))
            return false;
    }
    return true;
}

auto PetriNet::initial(size_t id) const -> MarkVal { return _initialMarking[id]; }

auto PetriNet::make_initial_marking() const -> MarkVal * {
    MarkVal *marking = new MarkVal[_nplaces];
    std::copy(_initialMarking, _initialMarking + _nplaces, marking);
    return marking;
}

void PetriNet::sort() {
    for (size_t i = 0; i < _ntransitions; ++i) {
        trans_ptr_t &t = _transitions[i];
        std::sort(&_invariants[t._inputs], &_invariants[t._outputs],
                  [](const auto &a, const auto &b) { return a._place < b._place; });
        trans_ptr_t &t2 = _transitions[i + 1];
        std::sort(&_invariants[t._outputs], &_invariants[t2._inputs],
                  [](const auto &a, const auto &b) { return a._place < b._place; });
    }
}

void PetriNet::to_xml(std::ostream &out) {
    out << "<?xml version=\"1.0\"?>\n"
        << "<pnml xmlns=\"http://www.pnml.org/version-2009/grammar/pnml\">\n"
        << "<net id=\"ClientsAndServers-PT-N0500P0\" "
           "type=\"http://www.pnml.org/version-2009/grammar/ptnet\">\n";
    out << "<page id=\"page0\">\n"
        << "<name>\n"
        << "<text>DefaultPage</text>"
        << "</name>";

    for (size_t i = 0; i < _nplaces; ++i) {
        auto &p = _placenames[i];
        auto &placelocation = _placelocations[i];
        out << "<place id=\"" << p << "\">\n"
            << "<graphics><position x=\"" << std::get<0>(placelocation) << "\" y=\""
            << std::get<1>(placelocation) << "\"/></graphics>\n"
            << "<name><text>" << p << "</text></name>\n";
        if (_initialMarking[i] > 0) {
            out << "<initialMarking><text>" << _initialMarking[i] << "</text></initialMarking>\n";
        }
        out << "</place>\n";
    }
    for (size_t i = 0; i < _ntransitions; ++i) {
        auto &transitionlocation = _transitionlocations[i];
        out << "<transition id=\"" << _transitionnames[i] << "\">\n"
            << "<name><text>" << _transitionnames[i] << "</text></name>\n";
        out << "<graphics><position x=\"" << std::get<0>(transitionlocation) << "\" y=\""
            << std::get<1>(transitionlocation) << "\"/></graphics>\n";
        out << "</transition>\n";
    }
    size_t id = 0;
    for (size_t t = 0; t < _ntransitions; ++t) {
        auto pre = preset(t);

        for (; pre.first != pre.second; ++pre.first) {
            out << "<arc id=\"" << (id++) << "\" source=\"" << _placenames[pre.first->_place]
                << "\" target=\"" << _transitionnames[t] << "\" type=\""
                << (pre.first->_inhibitor ? "inhibitor" : "normal") << "\">\n";

            if (pre.first->_tokens > 1) {
                out << "<inscription><text>" << pre.first->_tokens << "</text></inscription>\n";
            }

            out << "</arc>\n";
        }

        auto post = postset(t);
        for (; post.first != post.second; ++post.first) {
            out << "<arc id=\"" << (id++) << "\" source=\"" << _transitionnames[t] << "\" target=\""
                << _placenames[post.first->_place] << "\">\n";

            if (post.first->_tokens > 1) {
                out << "<inscription><text>" << post.first->_tokens << "</text></inscription>\n";
            }

            out << "</arc>\n";
        }
    }
    out << "</page></net>\n</pnml>";
}

} // namespace PetriEngine
