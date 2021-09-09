/*
 *  Copyright Peter G. Jensen, all rights reserved.
 */

/*
 * File:   range.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on March 31, 2020, 4:32 PM
 */

#ifndef RANGE_H
#define RANGE_H

#include <cassert>
#include <cinttypes>
#include <iostream>
#include <limits>
#include <vector>

namespace PetriEngine::Reachability {

struct range_t {

    static inline auto min() -> uint32_t { return std::numeric_limits<uint32_t>::min(); }

    static inline auto max() -> uint32_t { return std::numeric_limits<uint32_t>::max(); }

    range_t() = default;

    explicit range_t(uint32_t val) : _lower(val), _upper(val) {}

    range_t(uint32_t l, uint32_t u) : _lower(l), _upper(u) { assert(_lower <= _upper); }

    uint32_t _lower = min();
    uint32_t _upper = max();

    [[nodiscard]] auto no_upper() const -> bool { return _upper == max(); }

    [[nodiscard]] auto no_lower() const -> bool { return _lower == min(); }

    [[nodiscard]] auto unbound() const -> bool { return no_lower() && no_upper(); }

    [[nodiscard]] auto is_sound() const -> bool { return _lower <= _upper; }

    [[nodiscard]] auto contains(uint32_t id) const -> bool { return _lower <= id && id <= _upper; }

    void free() {
        _upper = max();
        _lower = min();
    }

    [[nodiscard]] auto size() const -> uint32_t { return 1 + _upper - _lower; }

    void invalidate() {
        // hack setting range invalid
        _lower = 1;
        _upper = 0;
    }

    auto print(std::ostream &os) const -> std::ostream & {
        if (no_lower())
            os << "[0";
        else
            os << "[" << _lower;
        os << ", ";
        if (no_upper())
            os << "inf]";
        else
            os << _upper << "]";
        return os;
    }

    [[nodiscard]] auto compare(const range_t &other) const -> std::pair<bool, bool> {
        return std::make_pair(_lower <= other._lower && _upper >= other._upper,
                              _lower >= other._lower && _upper <= other._upper);
    }

    [[nodiscard]] auto intersects(const range_t &other) const -> bool {
        return _lower <= other._upper && other._lower <= _upper;
    }

    auto operator&=(const range_t &other) -> range_t & {
        _lower = std::max(_lower, other._lower);
        _upper = std::min(_upper, other._upper);
        return *this;
    }

    auto operator|=(const range_t &other) -> range_t & {
        _lower = std::min(_lower, other._lower);
        _upper = std::max(_upper, other._upper);
        return *this;
    }

    auto operator|=(uint32_t val) -> range_t & {
        _lower = std::min(val, _lower);
        _upper = std::max(val, _upper);
        return *this;
    }

    auto operator&=(uint32_t val) -> range_t & {
        _lower = val;
        _upper = val;
        return *this;
    }

    auto operator-=(uint32_t val) -> range_t & {
        if (_lower < min() + val)
            _lower = min();
        else
            _lower -= val;
        if (_upper != max()) {
            if (_upper < min() + val) {
                assert(false);
            } else {
                _upper -= val;
            }
        }
        return *this;
    }

    auto operator+=(uint32_t val) -> range_t & {
        if (_lower != min()) {
            if (_lower >= max() - val)
                assert(false);
            _lower += val;
        }

        if (_upper >= max() - val) {
            _upper = max();
        } else
            _upper += val;
        return *this;
    }
};

struct placerange_t {
    range_t _range;
    uint32_t _place = std::numeric_limits<uint32_t>::max();

    placerange_t() = default;

    placerange_t(uint32_t place) : _place(place){};

    placerange_t(uint32_t place, const range_t &r) : _range(r), _place(place){};

    placerange_t(uint32_t place, range_t &&r) : _range(r), _place(place){};

    placerange_t(uint32_t place, uint32_t v) : _range(v), _place(place){};

    placerange_t(uint32_t place, uint32_t l, uint32_t u) : _range(l, u), _place(place){};

    auto print(std::ostream &os) const -> std::ostream & {
        os << "<P" << _place << "> in ";
        return _range.print(os);
    }

    [[nodiscard]] auto compare(const range_t &other) const -> std::pair<bool, bool> {
        return _range.compare(other);
    }

    [[nodiscard]] auto compare(const placerange_t &other) const -> std::pair<bool, bool> {
        assert(other._place == _place);
        if (other._place != _place)
            return std::make_pair(false, false);
        return _range.compare(other._range);
    }

    auto operator|=(uint32_t val) -> placerange_t & {
        _range |= val;
        return *this;
    }

    auto operator&=(uint32_t val) -> placerange_t & {
        _range &= val;
        return *this;
    }

    auto operator-=(uint32_t val) -> placerange_t & {
        _range -= val;
        return *this;
    }

    auto operator+=(uint32_t val) -> placerange_t & {
        _range += val;
        return *this;
    }

    auto operator&=(const placerange_t &other) -> placerange_t & {
        assert(other._place == _place);
        _range &= other._range;
        return *this;
    }

    auto operator|=(const placerange_t &other) -> placerange_t & {
        assert(other._place == _place);
        _range |= other._range;
        return *this;
    }

    // used for sorting only!

    auto operator<(const placerange_t &other) const -> bool { return _place < other._place; }
};

struct prvector_t {
    std::vector<placerange_t> _ranges;

    auto operator[](uint32_t place) const -> const placerange_t * {
        auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
        if (lb == _ranges.end() || lb->_place != place) {
            return nullptr;
        } else {
            return &(*lb);
        }
    }

    auto operator[](uint32_t place) -> placerange_t * {
        auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
        if (lb == _ranges.end() || lb->_place != place) {
            return nullptr;
        } else {
            return &(*lb);
        }
    }

    auto find_or_add(uint32_t place) -> placerange_t & {
        auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), place);
        if (lb == _ranges.end() || lb->_place != place) {
            lb = _ranges.emplace(lb, place);
        }
        return *lb;
    }

    [[nodiscard]] auto lower(uint32_t place) const -> uint32_t {
        auto *pr = (*this)[place];
        if (pr == nullptr)
            return range_t::min();
        return pr->_range._lower;
    }

    [[nodiscard]] auto upper(uint32_t place) const -> uint32_t {
        auto *pr = (*this)[place];
        if (pr == nullptr)
            return range_t::max();
        return pr->_range._upper;
    }

    [[nodiscard]] auto unbound(uint32_t place) const -> bool {
        auto *pr = (*this)[place];
        if (pr == nullptr)
            return true;
        return pr->_range.unbound();
    }

    void copy(const prvector_t &other) {
        _ranges = other._ranges;
        compact();
    }

    void compact() {
        for (int64_t i = _ranges.size() - 1; i >= 0; --i) {
            if (_ranges[i]._range.unbound())
                _ranges.erase(_ranges.begin() + i);
        }
        assert(is_compact());
    }

    [[nodiscard]] auto is_compact() const -> bool {
        for (auto &e : _ranges)
            if (e._range.unbound())
                return false;
        return true;
    }

    void compress() {
        int64_t i = _ranges.size();
        for (--i; i >= 0; --i)
            if (_ranges[i]._range.unbound())
                _ranges.erase(_ranges.begin() + i);
    }

    [[nodiscard]] auto compare(const prvector_t &other) const -> std::pair<bool, bool> {
        assert(is_compact());
        assert(other.is_compact());
        auto sit = _ranges.begin();
        auto oit = other._ranges.begin();
        std::pair<bool, bool> incl = std::make_pair(true, true);

        while (true) {
            if (sit == _ranges.end()) {
                incl.second = incl.second && (oit == other._ranges.end());
                break;
            } else if (oit == other._ranges.end()) {
                incl.first = false;
                break;
            } else if (sit->_place == oit->_place) {
                auto r = sit->compare(*oit);
                incl.first = incl.first && r.first;
                incl.second &= incl.second && r.second;
                ++sit;
                ++oit;
            } else if (sit->_place < oit->_place) {
                incl.first = false;
                ++sit;
            } else if (sit->_place > oit->_place) {
                incl.second = false;
                ++oit;
            }
            if (!incl.first && !incl.second)
                return incl;
        }
        return incl;
    }

    auto print(std::ostream &os) const -> std::ostream & {

        os << "{\n";
        for (auto &pr : _ranges) {
            os << "\t";
            pr.print(os) << "\n";
        }
        os << "}\n";
        return os;
    }

    [[nodiscard]] auto is_true() const -> bool { return _ranges.empty(); }

    [[nodiscard]] auto is_false(size_t nplaces) const -> bool {
        if (_ranges.size() != nplaces)
            return false;
        for (auto &p : _ranges) {
            if (p._range._lower != 0 || p._range._upper != 0)
                return false;
        }
        return true;
    }

    auto operator<(const prvector_t &other) const -> bool {
        if (_ranges.size() != other._ranges.size())
            return _ranges.size() < other._ranges.size();
        for (size_t i = 0; i < _ranges.size(); ++i) {
            auto &r = _ranges[i];
            auto &otr = other._ranges[i];

            if (r._place != otr._place)
                return r._place < otr._place;
            if (r._range._lower != otr._range._lower)
                return r._range._lower < otr._range._lower;
            if (r._range._upper != otr._range._upper)
                return r._range._upper < otr._range._upper;
        }
        return false;
    }

    auto operator==(const prvector_t &other) const -> bool {
        auto r = compare(other);
        return r.first && r.second;
    }

    auto operator&=(const placerange_t &other) -> prvector_t & {
        auto lb = std::lower_bound(_ranges.begin(), _ranges.end(), other);
        if (lb == std::end(_ranges) || lb->_place != other._place) {
            _ranges.insert(lb, other);
        } else {
            *lb &= other;
        }
        return *this;
    }

    auto operator&=(const prvector_t &other) -> prvector_t & {
        auto oit = other._ranges.begin();
        auto sit = _ranges.begin();
        while (sit != _ranges.end()) {
            while (oit != other._ranges.end() && oit->_place < sit->_place) {
                sit = _ranges.insert(sit, *oit);
                ++sit;
                ++oit;
            }
            if (oit == other._ranges.end() || oit->_place != sit->_place) {
                ++sit;
            } else {
                *sit &= *oit;
                ++sit;
            }
        }
        if (oit != other._ranges.end()) {
            _ranges.insert(_ranges.end(), oit, other._ranges.end());
        }
        return *this;
    }

    [[nodiscard]] auto restricts(const std::vector<int64_t> &writes) const -> bool {
        auto rit = _ranges.begin();
        for (auto p : writes) {
            while (rit != std::end(_ranges) && (rit->_place < p || rit->_range.unbound()))
                ++rit;
            if (rit == std::end(_ranges))
                break;
            if (rit->_place == p)
                return true;
        }
        return false;
    }
};
} // namespace PetriEngine::Reachability

#endif /* RANGE_H */
