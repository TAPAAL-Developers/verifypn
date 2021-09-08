/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
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

#ifndef INTERVALS_H
#define INTERVALS_H

#include "../TAR/range.h"
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>

namespace PetriEngine::Colored {

struct interval_t {
    std::vector<Reachability::range_t> _ranges;

    interval_t() = default;

    ~interval_t() = default;

    interval_t(const std::vector<Reachability::range_t> &ranges) : _ranges(ranges) {}

    [[nodiscard]] auto size() const -> size_t { return _ranges.size(); }

    [[nodiscard]] auto is_sound() const -> bool {
        for (const auto &range : _ranges) {
            if (!range.is_sound()) {
                return false;
            }
        }
        return true;
    }

    void add_range(Reachability::range_t &&newRange) { _ranges.emplace_back(newRange); }

    void add_range(const Reachability::range_t &newRange) { _ranges.emplace_back(newRange); }

    void add_range(Reachability::range_t newRange, uint32_t index) {
        _ranges.insert(_ranges.begin() + index, newRange);
    }

    void add_range(uint32_t l, uint32_t u) { _ranges.emplace_back(l, u); }

    auto operator[](size_t index) -> Reachability::range_t & {
        assert(index < _ranges.size());
        return _ranges[index];
    }

    auto operator[](size_t index) const -> const Reachability::range_t & { return _ranges[index]; }

    [[nodiscard]] auto get_lower_ids() const -> std::vector<uint32_t> {
        std::vector<uint32_t> ids;
        for (auto &range : _ranges) {
            ids.push_back(range._lower);
        }
        return ids;
    }

    [[nodiscard]] auto get_single_color_interval() const -> interval_t {
        interval_t newInterval;
        for (auto &range : _ranges) {
            newInterval.add_range(range._lower, range._lower);
        }
        return newInterval;
    }

    [[nodiscard]] auto equals(const interval_t &other) const -> bool {
        if (other.size() != size()) {
            return false;
        }
        for (uint32_t i = 0; i < size(); i++) {
            auto comparisonRes = _ranges[i].compare(other[i]);
            if (!comparisonRes.first || !comparisonRes.second) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] auto get_contained_colors() const -> uint32_t {
        uint32_t colors = 1;
        for (const auto &range : _ranges) {
            colors *= 1 + range._upper - range._lower;
        }
        return colors;
    }

    [[nodiscard]] auto contains(const interval_t &other,
                                const std::vector<bool> &diagonalPositions) const -> bool {
        if (other.size() != size()) {
            return false;
        }
        for (uint32_t i = 0; i < size(); i++) {
            if (!diagonalPositions[i] && !_ranges[i].compare(other[i]).first) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] auto get_overlap(const interval_t &other) const -> interval_t {
        interval_t overlapInterval;
        if (size() != other.size()) {
            return overlapInterval;
        }

        for (uint32_t i = 0; i < size(); i++) {
            auto rangeCopy = _ranges[i];
            overlapInterval.add_range(rangeCopy &= other[i]);
        }

        return overlapInterval;
    }

    [[nodiscard]] auto get_overlap(const interval_t &other,
                                   const std::vector<bool> &diagonalPositions) const -> interval_t {
        interval_t overlapInterval;
        if (size() != other.size()) {
            return overlapInterval;
        }

        for (uint32_t i = 0; i < size(); i++) {
            if (diagonalPositions[i]) {
                overlapInterval.add_range(_ranges[i]);
            } else {
                auto rangeCopy = _ranges[i];
                overlapInterval.add_range(rangeCopy &= other[i]);
            }
        }

        return overlapInterval;
    }

    auto operator|=(const interval_t &other) -> interval_t & {
        assert(size() == other.size());
        for (uint32_t l = 0; l < size(); ++l) {
            _ranges[l] |= other[l];
        }
        return *this;
    }

    [[nodiscard]] auto intersects(const interval_t &otherInterval) const -> bool {
        assert(size() == otherInterval.size());
        for (uint32_t k = 0; k < size(); k++) {
            if (!_ranges[k].intersects(otherInterval[k])) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] auto get_substracted(const interval_t &other,
                                       const std::vector<bool> &diagonalPositions) const
        -> std::vector<interval_t> {
        std::vector<interval_t> result;

        if (size() != other.size()) {
            return result;
        }

        for (uint32_t i = 0; i < size(); i++) {
            interval_t newInterval = *this;
            if (diagonalPositions[i]) {
                continue;
            }

            int32_t newMinUpper = std::min(((int)other[i]._lower) - 1, (int)_ranges[i]._upper);
            uint32_t newMaxLower = std::max(other[i]._upper + 1, _ranges[i]._lower);

            if (((int32_t)_ranges[i]._lower) <= newMinUpper && newMaxLower <= _ranges[i]._upper) {
                auto intervalCopy = *this;
                auto lowerRange = Reachability::range_t(_ranges[i]._lower, newMinUpper);
                auto upperRange = Reachability::range_t(newMaxLower, _ranges[i]._upper);
                newInterval._ranges[i] = lowerRange;
                intervalCopy._ranges[i] = upperRange;
                result.push_back(std::move(intervalCopy));
                result.push_back(std::move(newInterval));

            } else if (((int32_t)_ranges[i]._lower) <= newMinUpper) {
                auto lowerRange = Reachability::range_t(_ranges[i]._lower, newMinUpper);
                newInterval._ranges[i] = lowerRange;
                result.push_back(std::move(newInterval));
            } else if (newMaxLower <= _ranges[i]._upper) {
                auto upperRange = Reachability::range_t(newMaxLower, _ranges[i]._upper);
                newInterval._ranges[i] = upperRange;
                result.push_back(std::move(newInterval));
            }
        }
        return result;
    }

    void print() const {
        for (const auto &range : _ranges) {
            std::cout << " " << range._lower << "-" << range._upper << " ";
        }
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream strs;
        for (const auto &range : _ranges) {
            strs << " " << range._lower << "-" << range._upper << " ";
        }
        return strs.str();
    }
};

struct interval_dist_t {
    uint32_t _intervalId1;
    uint32_t _intervalId2;
    uint32_t _distance;
};

class IntervalVector {
  private:
    std::vector<interval_t> _intervals;

  public:
    ~IntervalVector() = default;

    IntervalVector() = default;

    IntervalVector(const std::vector<interval_t> &ranges) : _intervals(ranges){};

    auto begin() -> std::vector<interval_t>::iterator { return _intervals.begin(); }
    auto end() -> std::vector<interval_t>::iterator { return _intervals.end(); }
    [[nodiscard]] auto begin() const -> std::vector<interval_t>::const_iterator {
        return _intervals.begin();
    }
    [[nodiscard]] auto end() const -> std::vector<interval_t>::const_iterator {
        return _intervals.end();
    }

    [[nodiscard]] auto empty() const -> bool { return _intervals.empty(); }

    void clear() { _intervals.clear(); }

    [[nodiscard]] auto front() const -> const interval_t & { return _intervals[0]; }

    [[nodiscard]] auto back() const -> const interval_t & { return _intervals.back(); }

    [[nodiscard]] auto size() const -> size_t { return _intervals.size(); }

    [[nodiscard]] auto tuple_size() const -> size_t { return _intervals[0].size(); }

    [[nodiscard]] auto get_contained_colors() const -> uint32_t {
        uint32_t colors = 0;
        for (const auto &interval : _intervals) {
            colors += interval.get_contained_colors();
        }
        return colors;
    }

    static auto shift_interval(uint32_t lower, uint32_t upper, uint32_t ctSize, int32_t modifier)
        -> std::pair<uint32_t, uint32_t> {
        int32_t lower_val = ctSize + (lower + modifier);
        int32_t upper_val = ctSize + (upper + modifier);
        return std::make_pair(lower_val % ctSize, upper_val % ctSize);
    }

    [[nodiscard]] auto has_valid_intervals() const -> bool {
        for (const auto &interval : _intervals) {
            if (interval.is_sound()) {
                return true;
            }
        }
        return false;
    }

    auto operator[](size_t index) const -> const interval_t & {
        assert(index < _intervals.size());
        return _intervals[index];
    }

    auto operator[](size_t index) -> interval_t & {
        assert(index < _intervals.size());
        return _intervals[index];
    }

    void append(const IntervalVector &other) {
        _intervals.insert(_intervals.end(), other._intervals.begin(), other._intervals.end());
    }

    [[nodiscard]] auto is_range_end(const std::vector<uint32_t> &ids) const -> interval_t {
        for (uint32_t j = 0; j < _intervals.size(); j++) {
            bool rangeEnd = true;
            for (uint32_t i = 0; i < _intervals[j].size(); i++) {
                auto range = _intervals[j][i];
                if (range._upper != ids[i]) {
                    rangeEnd = false;
                    break;
                }
            }
            if (rangeEnd) {
                if (j + 1 != _intervals.size()) {
                    return _intervals[j + 1];
                } else {
                    return front();
                }
            }
        }
        return interval_t();
    }

    [[nodiscard]] auto shrink_intervals(uint32_t newSize) const
        -> std::vector<Colored::interval_t> {
        std::vector<Colored::interval_t> resizedIntervals;
        for (auto &interval : _intervals) {
            Colored::interval_t resizedInterval;
            for (uint32_t i = 0; i < newSize; i++) {
                resizedInterval.add_range(interval[i]);
            }
            resizedIntervals.push_back(resizedInterval);
        }
        return resizedIntervals;
    }

    void add_interval(const interval_t &interval) {
        uint32_t vecIndex = 0;

        if (!_intervals.empty()) {
            assert(_intervals[0].size() == interval.size());
        } else {
            _intervals.push_back(interval);
            return;
        }

        for (auto &localInterval : _intervals) {
            bool extendsInterval = true;
            enum found_place_e { UNDECIDED, GREATER, LOWER };
            found_place_e foundPlace = UNDECIDED;

            for (uint32_t k = 0; k < interval.size(); k++) {
                if (interval[k]._lower > localInterval[k]._upper ||
                    localInterval[k]._lower > interval[k]._upper) {
                    extendsInterval = false;
                }
                if (interval[k]._lower < localInterval[k]._lower) {
                    if (foundPlace == UNDECIDED) {
                        foundPlace = LOWER;
                    }
                } else if (interval[k]._lower > localInterval[k]._lower) {
                    if (foundPlace == UNDECIDED) {
                        foundPlace = GREATER;
                    }
                }
                if (!extendsInterval && foundPlace != UNDECIDED) {
                    break;
                }
            }

            if (extendsInterval) {
                for (uint32_t k = 0; k < interval.size(); k++) {
                    localInterval[k] |= interval[k];
                }
                return;
            } else if (foundPlace == LOWER) {
                break;
            }
            vecIndex++;
        }

        _intervals.insert(_intervals.begin() + vecIndex, interval);
    }

    void constrain_lower(const std::vector<uint32_t> &values, bool strict) {
        for (auto &_interval : _intervals) {
            for (uint32_t j = 0; j < values.size(); ++j) {
                if (strict && _interval[j]._lower <= values[j]) {
                    _interval[j]._lower = values[j] + 1;
                } else if (!strict && _interval[j]._lower < values[j]) {
                    _interval[j]._lower = values[j];
                }
            }
        }
        simplify();
    }

    void constrain_upper(const std::vector<uint32_t> &values, bool strict) {
        for (auto &_interval : _intervals) {
            for (uint32_t j = 0; j < values.size(); ++j) {
                if (strict && _interval[j]._upper >= values[j]) {
                    _interval[j]._upper = values[j] - 1;
                } else if (!strict && _interval[j]._upper > values[j]) {
                    _interval[j]._upper = values[j];
                }
            }
        }
        simplify();
    }

    void print() const {
        for (const auto &interval : _intervals) {
            std::cout << "[";
            interval.print();
            std::cout << "]" << std::endl;
        }
    }

    [[nodiscard]] auto to_string() const -> std::string {
        std::string out;
        for (const auto &interval : _intervals) {
            out += "[";
            out += interval.to_string();
            out += "]\n";
        }
        return out;
    }

    [[nodiscard]] auto get_lower_ids() const -> std::vector<uint32_t> {
        std::vector<uint32_t> ids;
        for (const auto &interval : _intervals) {
            if (ids.empty()) {
                ids = interval.get_lower_ids();
            } else {
                for (uint32_t i = 0; i < ids.size(); i++) {
                    ids[i] = std::min(ids[i], interval[i]._lower);
                }
            }
        }
        return ids;
    }

    [[nodiscard]] auto get_lower_ids(int32_t modifier, const std::vector<size_t> &sizes) const
        -> std::vector<uint32_t> {
        std::vector<uint32_t> ids;
        for (uint32_t j = 0; j < size(); j++) {
            auto &interval = _intervals[j];
            if (ids.empty()) {
                for (uint32_t i = 0; i < ids.size(); i++) {
                    auto shiftedInterval =
                        shift_interval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);
                    if (shiftedInterval.first > shiftedInterval.second) {
                        ids.push_back(0);
                    } else {
                        ids.push_back(shiftedInterval.first);
                    }
                }
            } else {
                for (uint32_t i = 0; i < ids.size(); i++) {
                    if (ids[i] == 0) {
                        continue;
                    }
                    auto shiftedInterval =
                        shift_interval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);
                    if (shiftedInterval.first > shiftedInterval.second) {
                        ids[i] = 0;
                    } else {
                        ids[i] = std::max(ids[i], shiftedInterval.first);
                    }
                }
            }
        }
        return ids;
    }

    [[nodiscard]] auto get_upper_ids(int32_t modifier, const std::vector<size_t> &sizes) const
        -> std::vector<uint32_t> {
        std::vector<uint32_t> ids;
        for (uint32_t j = 0; j < size(); j++) {
            const auto &interval = _intervals[j];
            if (ids.empty()) {
                for (uint32_t i = 0; i < ids.size(); i++) {
                    auto shiftedInterval =
                        shift_interval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);

                    if (shiftedInterval.first > shiftedInterval.second) {
                        ids.push_back(sizes[i] - 1);
                    } else {
                        ids.push_back(shiftedInterval.second);
                    }
                }
            } else {
                for (uint32_t i = 0; i < ids.size(); i++) {
                    if (ids[i] == sizes[i] - 1) {
                        continue;
                    }
                    auto shiftedInterval =
                        shift_interval(interval[i]._lower, interval[i]._upper, sizes[i], modifier);

                    if (shiftedInterval.first > shiftedInterval.second) {
                        ids[i] = sizes[i] - 1;
                    } else {
                        ids[i] = std::max(ids[i], shiftedInterval.second);
                    }
                }
            }
        }
        return ids;
    }

    void apply_modifier(int32_t modifier, const std::vector<size_t> &sizes) {
        std::vector<interval_t> collectedIntervals;
        for (auto &interval : _intervals) {
            std::vector<interval_t> newIntervals;
            newIntervals.push_back(std::move(interval));
            for (uint32_t i = 0; i < interval.size(); i++) {
                std::vector<interval_t> tempIntervals;
                for (auto &interval1 : newIntervals) {
                    auto shiftedInterval = shift_interval(interval1[i]._lower, interval1[i]._upper,
                                                          sizes[i], modifier);

                    if (shiftedInterval.first > shiftedInterval.second) {
                        auto newInterval = interval1;

                        interval1[i]._lower = 0;
                        interval1[i]._upper = shiftedInterval.second;

                        newInterval[i]._lower = shiftedInterval.first;
                        newInterval[i]._upper = sizes[i] - 1;
                        tempIntervals.push_back(std::move(newInterval));
                    } else {
                        interval1[i]._lower = shiftedInterval.first;
                        interval1[i]._upper = shiftedInterval.second;
                    }
                }
                newIntervals.insert(newIntervals.end(), tempIntervals.begin(), tempIntervals.end());
            }
            collectedIntervals.insert(collectedIntervals.end(), newIntervals.begin(),
                                      newIntervals.end());
        }

        _intervals = std::move(collectedIntervals);
    }

    [[nodiscard]] auto contains(const interval_t &interval,
                                const std::vector<bool> &diagonalPositions) const -> bool {
        for (const auto &localInterval : _intervals) {
            if (localInterval.contains(interval, diagonalPositions)) {
                return true;
            }
        }
        return false;
    }

    void remove_interval(uint32_t index) { _intervals.erase(_intervals.begin() + index); }

    void restrict(uint32_t k) {
        simplify();
        if (k == 0) {
            return;
        }

        while (size() > k) {
            interval_dist_t closestInterval = get_closest_intervals();
            auto &interval = _intervals[closestInterval._intervalId1];
            const auto &otherInterval = _intervals[closestInterval._intervalId2];

            for (uint32_t l = 0; l < interval.size(); l++) {
                interval[l] |= otherInterval[l];
            }

            _intervals.erase(_intervals.begin() + closestInterval._intervalId2);
        }
        simplify();
    }

    [[nodiscard]] auto get_closest_intervals() const -> interval_dist_t {
        interval_dist_t currentBest = {0, 0, std::numeric_limits<uint32_t>::max()};
        for (uint32_t i = 0; i < size() - 1; i++) {
            const auto &interval = _intervals[i];
            for (uint32_t j = i + 1; j < size(); j++) {
                const auto &otherInterval = _intervals[j];
                uint32_t dist = 0;

                for (uint32_t k = 0; k < interval.size(); k++) {
                    int32_t val1 = otherInterval[k]._lower - interval[k]._upper;
                    int32_t val2 = interval[k]._lower - otherInterval[k]._upper;
                    dist += std::max(0, std::max(val1, val2));
                    if (dist >= currentBest._distance) {
                        break;
                    }
                }

                if (dist < currentBest._distance) {
                    currentBest._distance = dist;
                    currentBest._intervalId1 = i;
                    currentBest._intervalId2 = j;

                    // if the distance is 1 we cannot find any intervals that are closer so we stop
                    // searching
                    if (currentBest._distance == 1) {
                        return currentBest;
                    }
                }
            }
        }
        return currentBest;
    }

    void simplify() {
        while (!_intervals.empty() && !_intervals[0].is_sound()) {
            _intervals.erase(_intervals.begin());
        }
        for (size_t i = 0; i < _intervals.size(); ++i) {
            for (size_t j = _intervals.size() - 1; j > i; --j) {
                const auto &otherInterval = _intervals[j];
                auto &interval = _intervals[i];
                if (!otherInterval.is_sound()) {
                    _intervals.erase(_intervals.begin() + j);
                } else if (interval.intersects(otherInterval)) {
                    interval |= otherInterval;
                    _intervals.erase(_intervals.begin() + j);
                }
            }
        }
    }

    void combine_neighbours() {
        std::set<uint32_t> rangesToRemove;
        if (_intervals.empty()) {
            return;
        }

        for (uint32_t i = 0; i < _intervals.size(); i++) {
            auto &interval = _intervals[i];
            if (!interval.is_sound()) {
                rangesToRemove.insert(i);
                continue;
            }
            for (uint32_t j = i + 1; j < _intervals.size(); j++) {
                const auto &otherInterval = _intervals[j];

                if (!otherInterval.is_sound()) {
                    continue;
                }
                bool overlap = true;

                uint32_t dist = 1;

                if (overlap) {
                    for (uint32_t k = 0; k < interval.size(); k++) {
                        if (interval[k]._lower > otherInterval[k]._upper ||
                            otherInterval[k]._lower > interval[k]._upper) {
                            if (interval[k]._lower > otherInterval[k]._upper + dist ||
                                otherInterval[k]._lower > interval[k]._upper + dist) {
                                overlap = false;
                                break;
                            } else {
                                dist = 0;
                            }
                        }
                    }
                }

                if (overlap) {
                    for (uint32_t l = 0; l < interval.size(); l++) {
                        interval[l] |= otherInterval[l];
                    }
                    rangesToRemove.insert(j);
                }
            }
        }
        for (auto i = rangesToRemove.rbegin(); i != rangesToRemove.rend(); ++i) {
            _intervals.erase(_intervals.begin() + *i);
        }
    }
};
} // namespace PetriEngine::Colored

#endif /* INTERVALS_H */