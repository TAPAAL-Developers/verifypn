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

#ifndef VERIFYPN_SUCCESSORQUEUE_H
#define VERIFYPN_SUCCESSORQUEUE_H

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <memory>

/**
 * LightDequeue derivative, designed for 'read-mostly' access
 * to list of successors. Popped elements are preserved post-mortem.
 */
template <typename T = uint32_t> class SuccessorQueue {
  public:
    SuccessorQueue(T *src, uint32_t nelem) : _size(nelem) {
        _data = std::make_unique<T[]>(nelem);
        memcpy(_data.get(), src, sizeof(T) * nelem);
    }

    // construct from array of different type, using fn as transformation function.
    template <typename U, typename Fn>
    SuccessorQueue(std::vector<U> &src, Fn &&fn) : _size(src.size()) {
        _data = std::make_unique<T[]>(src.size());
        std::transform(std::begin(src), std::end(src), _data.get(), fn);
    }

    SuccessorQueue() noexcept : _data(nullptr) {}

    [[nodiscard]] auto front() const -> T {
        assert(!empty());
        return _data[_front];
    }

    void pop() {
        assert(!empty());
        ++_front;
    }

    [[nodiscard]] auto size() const -> size_t { return _size - _front; }

    [[nodiscard]] auto empty() const -> bool { return _front >= _size; }

    [[nodiscard]] auto valid() const -> bool { return _data != nullptr; }

    [[nodiscard]] auto has_consumed() const -> bool { return _front > 0; }

    auto last_pop() const -> T {
        assert(has_consumed());
        return _data[_front - 1];
    }

    auto operator==(std::nullptr_t) -> bool { return _data == nullptr; }
    auto operator!=(std::nullptr_t) -> bool { return _data != nullptr; }

    /**
     * Extend successor list while excluding previously popped elements.
     * @param src C-array containing the new successor list
     * @param nelem the length of src
     */
    void extend_to(T *src, uint32_t nelem) {
        assert(nelem >= _size); // Cannot extend to fewer elements.
        auto newdata = std::make_unique<T[]>(nelem);
        if (_front != 0) {
            // Add previously fired transitions to start of array.
            memcpy(newdata.get(), _data.get(), sizeof(T) * _front);
        }
        uint32_t sz = _front;
        // copy over extended successor list, excluding previously popped elements.
        for (uint32_t i = 0; i < nelem; ++i) {
            auto begin = _data.get(), end = _data.get() + _front;
            auto it = std::find(begin, end, src[i]);

            // Transition not previously fired. Add it.
            if (it == end) {
                assert(sz < nelem);
                newdata[sz++] = src[i];
            }
        }
        _size = sz;
        assert(_size == nelem);
        _data.swap(newdata);
    }

    void append(T *src, uint32_t nelem) {
        auto newdata = std::make_unique<T[]>(nelem + _size);
        memcpy(newdata.get(), _data.get(), sizeof(T) * _size);
        memcpy(newdata.get() + _size, src, sizeof(T) * nelem);
        _size = nelem + _size;
        _data.swap(newdata);
    }

    auto all_successors() -> std::pair<T *, T *> {
        return std::make_pair(_data.get(), &_data[_size]);
    }

  private:
    uint32_t _front{0}; /* index of first element */
    uint32_t _size{0};  /* size of data array */
    std::unique_ptr<T[]> _data;
};

#endif // VERIFYPN_SUCCESSORQUEUE_H
