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

#include <algorithm>
#include <cassert>

#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PetriNet.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/Reducer.h"
#include "PetriEngine/TAR/ContainsVisitor.h"

namespace PetriEngine {

PetriNetBuilder::PetriNetBuilder() : AbstractPetriNetBuilder(), _reducer(this) {}
PetriNetBuilder::PetriNetBuilder(const PetriNetBuilder &other)
    : AbstractPetriNetBuilder(other), _placenames(other._placenames),
      _transitionnames(other._transitionnames), _placelocations(other._placelocations),
      _transitionlocations(other._transitionlocations), _transitions(other._transitions),
      _places(other._places), _initial_marking(other._initial_marking), _reducer(this) {}

void PetriNetBuilder::add_place(const std::string &name, uint32_t tokens, double x, double y) {
    if (_placenames.count(name) == 0) {
        uint32_t next = _placenames.size();
        _places.emplace_back();
        _placenames[name] = next;
        _placelocations.emplace_back(x, y);
    }

    uint32_t id = _placenames[name];

    while (_initial_marking.size() <= id)
        _initial_marking.emplace_back();
    _initial_marking[id] = tokens;
}

void PetriNetBuilder::add_transition(const std::string &name, double x, double y) {
    if (_transitionnames.count(name) == 0) {
        uint32_t next = _transitionnames.size();
        _transitions.emplace_back();
        _transitionnames[name] = next;
        _transitionlocations.emplace_back(x, y);
    }
}

void PetriNetBuilder::add_input_arc(const std::string &place, const std::string &transition,
                                    bool inhibitor, uint32_t weight) {
    if (_transitionnames.count(transition) == 0) {
        add_transition(transition, 0.0, 0.0);
    }
    if (_placenames.count(place) == 0) {
        add_place(place, 0, 0, 0);
    }
    uint32_t p = _placenames[place];
    uint32_t t = _transitionnames[transition];

    arc_t arc;
    arc._place = p;
    arc._weight = weight;
    arc._skip = false;
    arc._inhib = inhibitor;
    assert(t < _transitions.size());
    assert(p < _places.size());
    _transitions[t]._pre.push_back(arc);
    _transitions[t]._inhib |= inhibitor;
    _places[p]._consumers.push_back(t);
    _places[p]._inhib |= inhibitor;
}

void PetriNetBuilder::add_output_arc(const std::string &transition, const std::string &place,
                                     uint32_t weight) {
    if (_transitionnames.count(transition) == 0) {
        add_transition(transition, 0, 0);
    }
    if (_placenames.count(place) == 0) {
        add_place(place, 0, 0, 0);
    }
    uint32_t p = _placenames[place];
    uint32_t t = _transitionnames[transition];

    assert(t < _transitions.size());
    assert(p < _places.size());

    arc_t arc;
    arc._place = p;
    arc._weight = weight;
    arc._skip = false;
    _transitions[t]._post.push_back(arc);
    _places[p]._producers.push_back(t);
}

auto PetriNetBuilder::next_place_id(std::vector<uint32_t> &counts, std::vector<uint32_t> &pcounts,
                                    std::vector<uint32_t> &ids, bool reorder) -> uint32_t {
    uint32_t cand = std::numeric_limits<uint32_t>::max();
    uint32_t cnt = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < _places.size(); ++i) {
        uint32_t nnum =
            (pcounts[i] == 0 ? 0 : (counts[0] == 0 ? 0 : std::max(counts[i], pcounts[i])));
        if (ids[i] == std::numeric_limits<uint32_t>::max() && nnum < cnt && !_places[i]._skip) {
            if (!reorder)
                return i;
            cand = i;
            cnt = nnum;
        }
    }
    return cand;
}

auto PetriNetBuilder::make_petri_net(bool reorder) -> PetriNet * {

    /*
     * The basic idea is to construct three arrays, the first array,
     * _invariants points to "arcs" - they are triplets (weight, place, inhibitor)
     * _transitions are pairs, (input, output) are indexes in the _invariants array
     * _placeToPtrs is an indirection going from a place-index to the FIRST transition
     *              with a non-inhibitor arc consuming from the given place.
     *
     * For all the indexes and indirections, notice that we only track the
     * beginning. We can naturally use the "next" value as the end. eg. the
     * inputs of a transition are between "input" and "output". The outputs
     * are between "output" and the "input" of the next transition.
     *
     * This allows us to quickly skip a lot of checks when generating successors
     * Beware that currently "orphans" and "inhibitor orphans" are special-cases
     * and currently handled as "consuming" from place id=0.
     *
     * If anybody wants to spend time on it, this is the first step towards
     * a decision-tree like construction, possibly improving successor generation.
     */

    uint32_t nplaces = _places.size() - _reducer.removed_places();
    uint32_t ntrans = _transitions.size() - _reducer.removed_transitions();

    std::vector<uint32_t> place_cons_count = std::vector<uint32_t>(_places.size());
    std::vector<uint32_t> place_prod_count = std::vector<uint32_t>(_places.size());
    std::vector<uint32_t> place_idmap = std::vector<uint32_t>(_places.size());
    std::vector<uint32_t> trans_idmap = std::vector<uint32_t>(_transitions.size());

    uint32_t invariants = 0;

    for (uint32_t i = 0; i < _places.size(); ++i) {
        place_idmap[i] = std::numeric_limits<uint32_t>::max();
        if (!_places[i]._skip) {
            place_cons_count[i] = _places[i]._consumers.size();
            place_prod_count[i] = _places[i]._producers.size();
            invariants += _places[i]._consumers.size() + _places[i]._producers.size();
        }
    }

    for (uint32_t i = 0; i < _transitions.size(); ++i) {
        trans_idmap[i] = std::numeric_limits<uint32_t>::max();
    }

    PetriNet *net = new PetriNet(ntrans, invariants, nplaces);

    uint32_t next = next_place_id(place_cons_count, place_prod_count, place_idmap, reorder);
    uint32_t free = 0;
    uint32_t freeinv = 0;
    uint32_t freetrans = 0;

    // first handle orphans
    if (place_idmap.size() > next)
        place_idmap[next] = free;
    net->_placeToPtrs[free] = freetrans;
    for (size_t t = 0; t < _transitions.size(); ++t) {
        transition_t &trans = _transitions[t];
        if (std::all_of(trans._pre.begin(), trans._pre.end(), [](auto &a) { return a._inhib; })) {
            // ALL have to be inhibitor, if any. Otherwise not orphan

            if (trans._skip)
                continue;
            net->_transitions[freetrans]._inputs = freeinv;

            // add inhibitors
            for (auto pre : trans._pre) {
                invariant_t &iv = net->_invariants[freeinv];
                iv._place = pre._place;
                iv._tokens = pre._weight;
                iv._inhibitor = pre._inhib;
                assert(pre._inhib);
                assert(place_cons_count[pre._place] > 0);
                --place_cons_count[pre._place];
                ++freeinv;
            }

            net->_transitions[freetrans]._outputs = freeinv;

            for (auto post : trans._post) {
                assert(freeinv < net->_ninvariants);
                net->_invariants[freeinv]._place = post._place;
                net->_invariants[freeinv]._tokens = post._weight;
                ++freeinv;
            }

            trans_idmap[t] = freetrans;

            ++freetrans;
        }
    }

    bool first = true;
    while (next != std::numeric_limits<uint32_t>::max()) {
        if (first) // already set for first iteration to handle orphans
        {
            first = false;
        } else {
            place_idmap[next] = free;
            net->_placeToPtrs[free] = freetrans;
        }

        for (auto t : _places[next]._consumers) {
            auto &trans = _transitions[t];
            if (trans._skip)
                continue;

            net->_transitions[freetrans]._inputs = freeinv;

            // check first, we are going to change state later, but we can
            // break here, so no statechange inside loop!
            bool ok = true;
            bool all_inhib = true;
            uint32_t cnt = 0;
            for (const auto &pre : trans._pre) {
                all_inhib &= pre._inhib;

                // if transition belongs to previous place
                if ((!pre._inhib && place_idmap[pre._place] < free) ||
                    freeinv + cnt >= net->_ninvariants) {
                    ok = false;
                    break;
                }

                // or arc from place is an inhibitor
                if (pre._place == next && pre._inhib) {
                    ok = false;
                    break;
                }
                ++cnt;
            }

            // skip for now, either T-a->P is inhibitor, or was allready added for other P'
            // or all a's are inhibitors.
            if (!ok || all_inhib)
                continue;

            trans_idmap[t] = freeinv;

            // everything is good, change state!.
            for (auto pre : trans._pre) {
                invariant_t &iv = net->_invariants[freeinv];
                iv._place = pre._place;
                iv._tokens = pre._weight;
                iv._inhibitor = pre._inhib;
                ++freeinv;
                assert(place_cons_count[pre._place] > 0);
                --place_cons_count[pre._place];
            }

            net->_transitions[freetrans]._outputs = freeinv;
            for (auto post : trans._post) {
                assert(freeinv < net->_ninvariants);
                auto &post_inv = net->_invariants[freeinv];
                post_inv._place = post._place;
                post_inv._tokens = post._weight;
                --place_prod_count[post._place];
                ++freeinv;
            }

            trans_idmap[t] = freetrans;

            ++freetrans;
            assert(freeinv <= invariants);
        }
        ++free;
        next = next_place_id(place_cons_count, place_prod_count, place_idmap, reorder);
    }

    // Reindex for great justice!
    for (uint32_t i = 0; i < freeinv; i++) {
        net->_invariants[i]._place = place_idmap[net->_invariants[i]._place];
        assert(net->_invariants[i]._place < nplaces);
        assert(net->_invariants[i]._tokens > 0);
    }

    //        std::cout << "init" << std::endl;
    for (uint32_t i = 0; i < _places.size(); ++i) {
        if (place_idmap[i] != std::numeric_limits<uint32_t>::max()) {
            net->_initialMarking[place_idmap[i]] = _initial_marking[i];
            //                std::cout << place_idmap[i] << " : " << initialMarking[i] <<
            //                std::endl;
        }
    }

    net->_placelocations = _placelocations;
    net->_transitionlocations = _transitionlocations;

    // reindex place-names
    net->_placenames.resize(_placenames.size());
    int rindex = ((int)_placenames.size()) - 1;
    for (auto &i : _placenames) {
        auto &placelocation = _placelocations[i.second];
        i.second = place_idmap[i.second];
        if (i.second != std::numeric_limits<uint32_t>::max()) {
            net->_placenames[i.second] = i.first;
            assert(_placenames[net->_placenames[i.second]] == i.second);
            net->_placelocations[i.second] = placelocation;
        } else {
            net->_placenames[rindex] = i.first;
            net->_placelocations[rindex] = placelocation;
            --rindex;
        }
    }
    net->_transitionnames.resize(_transitionnames.size());
    int trindex = ((int)_transitionnames.size()) - 1;
    for (auto &i : _transitionnames) {
        auto &transitionlocation = _transitionlocations[i.second];
        i.second = trans_idmap[i.second];
        if (i.second != std::numeric_limits<uint32_t>::max()) {
            net->_transitionnames[i.second] = i.first;
            net->_transitionlocations[i.second] = transitionlocation;
        } else {
            net->_transitionnames[trindex] = i.first;
            net->_transitionlocations[trindex] = transitionlocation;
            --trindex;
        }
    }
    net->sort();

    for (size_t t = 0; t < net->number_of_transitions(); ++t) {
        {
            auto tiv = std::make_pair(&net->_invariants[net->_transitions[t]._inputs],
                                      &net->_invariants[net->_transitions[t]._outputs]);
            for (; tiv.first != tiv.second; ++tiv.first) {
                tiv.first->_direction = tiv.first->_inhibitor ? 0 : -1;
                bool found = false;
                auto tov = std::make_pair(&net->_invariants[net->_transitions[t]._outputs],
                                          &net->_invariants[net->_transitions[t + 1]._inputs]);
                for (; tov.first != tov.second; ++tov.first) {
                    if (tov.first->_place == tiv.first->_place) {
                        found = true;
                        if (tiv.first->_inhibitor)
                            tiv.first->_direction = tov.first->_direction = 1;
                        else if (tiv.first->_tokens < tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = 1;
                        else if (tiv.first->_tokens == tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = 0;
                        else if (tiv.first->_tokens > tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = -1;
                        break;
                    }
                }
                if (!found)
                    assert(tiv.first->_direction < 0 || tiv.first->_inhibitor);
            }
        }
        {
            auto tiv = std::make_pair(&net->_invariants[net->_transitions[t]._outputs],
                                      &net->_invariants[net->_transitions[t + 1]._inputs]);
            for (; tiv.first != tiv.second; ++tiv.first) {
                tiv.first->_direction = 1;
                bool found = false;
                auto tov = std::make_pair(&net->_invariants[net->_transitions[t]._inputs],
                                          &net->_invariants[net->_transitions[t]._outputs]);
                for (; tov.first != tov.second; ++tov.first) {
                    found = true;
                    if (tov.first->_place == tiv.first->_place) {
                        if (tov.first->_inhibitor)
                            tiv.first->_direction = tov.first->_direction = 1;
                        else if (tiv.first->_tokens > tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = 1;
                        else if (tiv.first->_tokens == tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = 0;
                        else if (tiv.first->_tokens < tov.first->_tokens)
                            tiv.first->_direction = tov.first->_direction = -1;
                        break;
                    }
                }
                if (!found)
                    assert(tiv.first->_direction > 0);
            }
        }
    }
    return net;
}

void PetriNetBuilder::sort() {
    for (auto &p : _places) {
        std::sort(p._consumers.begin(), p._consumers.end());
        std::sort(p._producers.begin(), p._producers.end());
    }

    for (auto &t : _transitions) {
        std::sort(t._pre.begin(), t._pre.end());
        std::sort(t._post.begin(), t._post.end());
    }
}

void PetriNetBuilder::reduce(std::vector<std::shared_ptr<PQL::Condition>> &queries,
                             std::vector<Reachability::ResultPrinter::result_e> &results,
                             int reductiontype, bool reconstructTrace, const PetriNet *net,
                             int timeout, std::vector<uint32_t> &reductions) {
    QueryPlaceAnalysisContext placecontext(get_place_names(), get_transition_names(), net);
    bool all_reach = true;
    bool remove_loops = true;
    bool contains_next = false;
    for (uint32_t i = 0; i < queries.size(); ++i) {
        if (results[i] == Reachability::ResultPrinter::UNKNOWN ||
            results[i] == Reachability::ResultPrinter::CTL ||
            results[i] == Reachability::ResultPrinter::LTL) {
            queries[i]->analyze(placecontext);
            all_reach &= (results[i] != Reachability::ResultPrinter::CTL &&
                          results[i] != Reachability::ResultPrinter::LTL);
            remove_loops &= !queries[i]->is_loop_sensitive();
            // There is a deadlock somewhere, if it is not alone, we cannot reduce.
            // this has similar problems as nested next.
            contains_next |= queries[i]->contains_next() || queries[i]->nested_deadlock();
        }
    }
    _reducer.reduce(placecontext, reductiontype, reconstructTrace, timeout, remove_loops, all_reach,
                    contains_next, reductions);
}

} // namespace PetriEngine
