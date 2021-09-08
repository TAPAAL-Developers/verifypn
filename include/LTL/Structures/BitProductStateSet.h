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

#ifndef VERIFYPN_BITPRODUCTSTATESET_H
#define VERIFYPN_BITPRODUCTSTATESET_H

#include "LTL/Structures/ProductState.h"
#include "PetriEngine/Structures/StateSet.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace LTL::Structures {

class ProductStateSetInterface {
  public:
    using stateid_t = size_t;
    using result_t = std::pair<bool, stateid_t>;

    virtual auto get_buchi_state(stateid_t id) -> size_t = 0;

    virtual auto get_marking_id(stateid_t id) -> size_t = 0;

    virtual auto get_product_id(size_t markingId, size_t buchiState) -> stateid_t = 0;

    virtual auto add(const LTL::Structures::ProductState &state) -> result_t = 0;

    virtual auto decode(LTL::Structures::ProductState &state, stateid_t id) -> bool = 0;

    virtual void set_history(stateid_t id, size_t transition) {}

    virtual auto get_history(stateid_t stateid) -> std::pair<size_t, size_t> {
        return std::make_pair(std::numeric_limits<size_t>::max(),
                              std::numeric_limits<size_t>::max());
    }

    [[nodiscard]] virtual auto discovered() const -> size_t = 0;

    [[nodiscard]] virtual auto max_tokens() const -> size_t = 0;

    virtual ~ProductStateSetInterface() = default;
};

/**
 * Bit-hacking product state set for storing pairs (M, q) compactly in 64 bits.
 * Allows for a max of 2^nbits Büchi states and 2^(64-nbits) markings without overflow.
 * @tparam nbits the number of bits to allocate for Büchi state. Defaults to 16-bit. Max is 32-bit.
 */
template <uint8_t nbits = 16> class BitProductStateSet : public ProductStateSetInterface {
  public:
    explicit BitProductStateSet(const PetriEngine::PetriNet &net, int kbound = 0,
                                size_t nplaces = -1)
        : _markings(net, kbound, net.number_of_places()) {}

    static_assert(nbits <= 32, "Only up to 2^32 Büchi states supported");
    // using stateid_t = size_t;

    /**
     * bool success
     * size_t stateID; if error it is UINT64_MAX.
     */

    auto get_buchi_state(stateid_t id) -> size_t override { return id >> _buchiShift; }

    auto get_marking_id(stateid_t id) -> size_t override { return id & _markingMask; }

    auto get_product_id(size_t markingId, size_t buchiState) -> stateid_t override {
        return (buchiState << _buchiShift) | (_markingMask & markingId);
    }

    /**
     * Insert a product state into the state set.
     * @param state the product state to insert.
     * @return pair of [success, ID]
     */
    auto add(const LTL::Structures::ProductState &state) -> result_t override {
        ++_discovered;
        const auto [_, markingId] = _markings.add(state);
        const stateid_t product_id = get_product_id(markingId, state.get_buchi_state());

        const auto [iter, is_new] = _states.insert(product_id);
        assert(iter != std::end(_states));
        return std::make_pair(is_new, product_id);
    }

    /**
     * Retrieve a product state from the state set.
     * @param id Composite state ID as previously generated by this.
     * @param state Output parameter to write product state to.
     * @return true if the state was successfully retrieved, false otherwise.
     */
    auto decode(LTL::Structures::ProductState &state, stateid_t id) -> bool override {
        const auto it = _states.find(id);
        if (it == std::cend(_states)) {
            return false;
        }
        auto marking_id = get_marking_id(*it);
        auto buchi_state = get_buchi_state(*it);
        _markings.decode(state, marking_id);
        state.set_buchi_state(buchi_state);
        return true;
    }

    // size_t size() { return states.size(); }
    auto discovered() const -> size_t override { return _discovered; }

    auto max_tokens() const -> size_t override { return _markings.max_tokens(); }

  protected:
    static constexpr auto _markingMask = (1LL << (64 - nbits)) - 1;
    static constexpr auto _buchiMask = std::numeric_limits<size_t>::max() ^ _markingMask;
    static constexpr auto _buchiShift = 64 - nbits;

    PetriEngine::Structures::StateSet _markings;
    std::unordered_set<stateid_t> _states;
    static constexpr auto _err_val = std::make_pair(false, std::numeric_limits<size_t>::max());

    size_t _discovered = 0;
};

template <uint8_t nbytes = 16>
class TraceableBitProductStateSet : public BitProductStateSet<nbytes> {
    using stateid_t = typename BitProductStateSet<nbytes>::stateid_t;

  public:
    explicit TraceableBitProductStateSet(const PetriEngine::PetriNet &net, int kbound = 0)
        : BitProductStateSet<nbytes>(net, kbound) {}

    auto decode(ProductState &state, stateid_t id) -> bool override {
        _parent = id;
        return BitProductStateSet<nbytes>::decode(state, id);
    }

    void set_history(stateid_t id, size_t transition) override {
        _history[id] = {_parent, transition};
    }

    auto get_history(stateid_t stateid) -> std::pair<size_t, size_t> override {
        auto [parent, trans] = _history.at(stateid);
        return std::make_pair(parent, trans);
    }

  private:
    struct history_t {
        size_t _parent;
        size_t _trans;
    };
    stateid_t _parent = 0;
    // product ID to parent ID
    std::unordered_map<size_t, history_t> _history;
};
} // namespace LTL::Structures

#endif // VERIFYPN_BITPRODUCTSTATESET_H
