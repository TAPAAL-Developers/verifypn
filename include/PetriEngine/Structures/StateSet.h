/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2016  Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef STATESET_H
#define STATESET_H
#include "AlignedEncoder.h"
#include "BinaryWrapper.h"
#include "State.h"
#include "errorcodes.h"
#include <iostream>
#include <ptrie/ptrie_map.h>
#include <ptrie/ptrie_stable.h>
#include <unordered_map>

namespace PetriEngine::Structures {

class StateSetInterface {
  public:
    StateSetInterface(const PetriNet &net, uint32_t kbound, int nplaces = -1)
        : _nplaces(nplaces == -1 ? net.number_of_places() : nplaces), _encoder(_nplaces),
          _net(net) {
        _discovered = 0;
        _kbound = kbound;
        _maxTokens = 0;
        _maxPlaceBound = std::vector<uint32_t>(net.number_of_places(), 0);
        _sp = BinaryWrapper(sizeof(uint32_t) * _nplaces * 8);
    }

    virtual ~StateSetInterface() { _sp.release(); }

    virtual auto add(const State &state) -> std::pair<bool, size_t> = 0;

    virtual void decode(State &state, size_t id) = 0;

    virtual auto lookup(State &state) -> std::pair<bool, size_t> = 0;

    [[nodiscard]] auto net() const -> const PetriNet & { return _net; }

    virtual void set_history(size_t id, size_t transition) = 0;

    [[nodiscard]] virtual auto get_history(size_t markingid) const -> std::pair<size_t, size_t> = 0;

  protected:
    size_t _discovered;
    uint32_t _kbound;
    uint32_t _maxTokens;
    size_t _nplaces;
    std::vector<uint32_t> _maxPlaceBound;
    AlignedEncoder _encoder;
    const PetriNet &_net;
    BinaryWrapper _sp;
#ifdef DEBUG
    std::vector<uint32_t *> _dbg;
#endif
    template <typename T> void decode(State &state, size_t id, T &_trie) {
        _trie.unpack(id, _encoder.scratchpad().raw());
        _encoder.decode(state.marking(), _encoder.scratchpad().raw());

#ifdef DEBUG
        assert(memcmp(state.marking(), _dbg[id], sizeof(uint32_t) * _net.number_of_places()) == 0);
#endif
    }

    template <typename T> auto add(const State &state, T &_trie) -> std::pair<bool, size_t> {
        _discovered++;

#ifdef DEBUG
        if (_discovered % 1000000 == 0)
            std::cout << "Found number " << _discovered << std::endl;
#endif

        MarkVal sum = 0;
        bool allsame = true;
        uint32_t val = 0;
        uint32_t active = 0;
        uint32_t last = 0;
        marking_stats(state.marking(), sum, allsame, val, active, last, _nplaces);

        if (_maxTokens < sum)
            _maxTokens = sum;

        // Check that we're within k-bound
        if (_kbound != 0 && sum > _kbound)
            return std::pair<bool, size_t>(false, std::numeric_limits<size_t>::max());

        unsigned char type = _encoder.get_type(sum, active, allsame, val);

        size_t length = _encoder.encode(state.marking(), type);
        if (length * 8 >= std::numeric_limits<uint16_t>::max()) {
            throw base_error_t("error: Marking could not be encoded into less than 2^16 bytes, "
                             "current limit of PTries");
        }
        BinaryWrapper w = BinaryWrapper(_encoder.scratchpad().raw(), length * 8);
        auto tit = _trie.insert(w.raw(), w.size());

        if (!tit.first) {
            return std::pair<bool, size_t>(false, tit.second);
        }

#ifdef DEBUG
        _dbg.push_back(new uint32_t[_net.number_of_places()]);
        memcpy(_dbg.back(), state.marking(), _net.number_of_places() * sizeof(uint32_t));
        decode(state, _trie.size() - 1);
#endif

        // update the max token bound for each place in the net (only for newly discovered markings)
        for (uint32_t i = 0; i < _net.number_of_places(); i++) {
            _maxPlaceBound[i] = std::max<MarkVal>(state.marking()[i], _maxPlaceBound[i]);
        }

#ifdef DEBUG
        if (_trie.size() % 100000 == 0)
            std::cout << "Inserted " << _trie.size() << std::endl;
#endif
        return std::pair<bool, size_t>(true, tit.second);
    }

    template <typename T> auto lookup(const State &state, T &_trie) -> std::pair<bool, size_t> {
        MarkVal sum = 0;
        bool allsame = true;
        uint32_t val = 0;
        uint32_t active = 0;
        uint32_t last = 0;
        marking_stats(state.marking(), sum, allsame, val, active, last, _nplaces);

        unsigned char type = _encoder.get_type(sum, active, allsame, val);

        size_t length = _encoder.encode(state.marking(), type);
        BinaryWrapper w = BinaryWrapper(_encoder.scratchpad().raw(), length * 8);
        auto tit = _trie.exists(w.raw(), w.size());

        if (tit.first) {
            return tit;
        } else
            return std::make_pair(false, std::numeric_limits<size_t>::max());
    }

  public:
    [[nodiscard]] auto discovered() const -> size_t { return _discovered; }

    [[nodiscard]] auto max_tokens() const -> uint32_t { return _maxTokens; }

    [[nodiscard]] auto max_place_bound() const -> const std::vector<MarkVal> & {
        return _maxPlaceBound;
    }

    static void marking_stats(const uint32_t *marking, MarkVal &sum, bool &allsame, uint32_t &val,
                              uint32_t &active, uint32_t &last, size_t nplaces) {
        for (uint32_t i = 0; i < nplaces; i++) {
            uint32_t old = val;
            if (marking[i] != 0) {
                last = std::max(last, i);
                val = std::max(marking[i], val);
                if (old != 0 && marking[i] != old)
                    allsame = false;
                ++active;
                sum += marking[i];
            }
        }
    }
};

#define STATESET_BUCKETS 1000000
class StateSet : public StateSetInterface {
  private:
    using wrapper_t = ptrie::BinaryWrapper;
    using ptrie_t = ptrie::set_stable<ptrie::uchar, 17, 128, 4>;
    ptrie_t _trie;

  public:
    using StateSetInterface::StateSetInterface;

    auto add(const State &state) -> std::pair<bool, size_t> override { return StateSetInterface::add(state, _trie); }

    void decode(State &state, size_t id) override { StateSetInterface::decode(state, id, _trie); }

    auto lookup(State &state) -> std::pair<bool, size_t> override { return StateSetInterface::lookup(state, _trie); }

    void set_history(size_t id, size_t transition) override {}

    [[nodiscard]] auto get_history(size_t markingid) const -> std::pair<size_t, size_t> override {
        assert(false);
        return std::make_pair(0, 0);
    }
};

class TracableStateSet : public StateSetInterface {
  public:
    struct traceable_t {
        ptrie::uint _parent;
        ptrie::uint _transition = std::numeric_limits<ptrie::uint>::max();
    };

  private:
    using ptrie_t = ptrie::map<unsigned char, traceable_t>;

  public:
    using StateSetInterface::StateSetInterface;

    auto add(const State &state) -> std::pair<bool, size_t> override {
#ifdef DEBUG
        size_t tmppar = _parent; // we might change this while debugging.
#endif
        auto res = StateSetInterface::add(state, _trie);
#ifdef DEBUG
        _parent = tmppar;
#endif
        return res;
    }

    void decode(State &state, size_t id) override {
        _parent = id;
        StateSetInterface::decode(state, id, _trie);
    }

    auto lookup(State &state) -> std::pair<bool, size_t> override { return StateSetInterface::lookup(state, _trie); }

    void set_history(size_t id, size_t transition) override {
        traceable_t &t = _trie.get_data(id);
        t._parent = _parent;
        t._transition = transition;
    }

    [[nodiscard]] auto get_history(size_t markingid) const -> std::pair<size_t, size_t> override {
        // const-cast here until the ptrie-library supports const access
        traceable_t &t = const_cast<ptrie_t &>(_trie).get_data(markingid);
        return std::pair<size_t, size_t>(t._parent, t._transition);
    }

  private:
    ptrie_t _trie;
    size_t _parent = 0;
};

} // namespace PetriEngine::Structures

#endif // STATESET_H
