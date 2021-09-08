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

/*
 * File:   binarywrapper.h
 * Author: Peter G. Jensen
 *
 * Created on 10 June 2015, 19:20
 */

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

#ifndef BINARYWRAPPER_H
#define BINARYWRAPPER_H

namespace ptrie {
using uint = unsigned int;
using uchar = unsigned char;
constexpr auto BW_BSIZE = sizeof(size_t); // SIZE OF POINTER!
/**
 * Wrapper for binary data. This provides easy access to individual bits,
 * heap allocation and comparison. Notice that one has to make sure to
 * explicitly call release() if one wishes to deallocate (possibly shared data).
 *
 */
class BinaryWrapper {
  public:
    // Constructors
    /**
     * Empty constructor, no data is allocated
     */

    BinaryWrapper() {}

    /**
     Allocates a room for at least size bits
     */

    BinaryWrapper(uint size);

    /**
     * Constructor for copying over data from latest the offset'th bit.
     * Detects overflows.
     * @param other: wrapper to copy from
     * @param offset: maximal number of bits to skip.
     */

    BinaryWrapper(const BinaryWrapper &other, uint offset);

    inline void init(const BinaryWrapper &other, uint size, uint offset, uint encodingsize) {
        uint so = size + offset;
        offset = ((so - 1) / 8) - ((size - 1) / 8);

        _nbytes = ((encodingsize + this->overhead(encodingsize)) / 8);
        if (_nbytes > offset)
            _nbytes -= offset;
        else {
            _nbytes = 0;
        }

        _blob = allocate(_nbytes);

        memcpy(raw(), &(other.const_raw()[offset]), _nbytes);
    }

    BinaryWrapper(uchar *raw, uint size, uint offset, uint encsize);

    /**
     * Assign (not copy) raw data to pointer. Set number of bytes to size
     * @param org: some memory to point to
     * @param size: number of bytes.
     */

    BinaryWrapper(uchar *org, uint size);

    /**
     * Empty destructor. Does NOT deallocate data - do this with explicit
     * call to release().
     */

    ~BinaryWrapper() = default;

    /**
     * Makes a complete copy, including new heap-allocation
     * @return an exact copy, but in a different area of the heap.
     */

    // BinaryWrapper clone() const;

    /**
     * Copy over data and meta-data from other, but insert only into target
     * after offset bits.
     * Notice that this can cause memory-corruption if there is not enough
     * room in target, or to many bits are skipped.
     * @param other: wrapper to copy from
     * @param offset: bits to skip
     */

    void copy(const BinaryWrapper &other, uint offset);

    /**
     * Copy over size bytes form raw data. Assumes that current wrapper has
     * enough room.
     * @param org: source data
     * @param size: number of bytes to copy
     */

    void copy(const uchar *org, uint size);

    // accessors
    /**
     * Get value of the place'th bit
     * @param place: bit index
     * @return
     */
    [[nodiscard]] inline auto at(const uint place) const -> bool {
        uint offset = place % 8;
        bool res2;
        if (place / 8 < _nbytes)
            res2 = (const_raw()[place / 8] & _masks[offset]) != 0;
        else
            res2 = false;

        return res2;
    }

    /**
     * number of bytes allocated in heap
     * @return
     */

    [[nodiscard]] inline auto size() const -> uint { return _nbytes; }

    /**
     * Raw access to data when in const setting
     * @return
     */

    [[nodiscard]] inline auto const_raw() const -> const uchar * {
        if (_nbytes <= BW_BSIZE)
            return offset((uchar *)&_blob, _nbytes);
        else
            return offset(_blob, _nbytes);
    }

    /**
     * Raw access to data
     * @return
     */

    inline auto raw() -> uchar * { return const_cast<uchar *>(const_raw()); }

    /**
     * pretty print of content
     */

    void print(std::ostream &strean, size_t length = std::numeric_limits<size_t>::max()) const;

    /**
     * finds the overhead (unused number of bits) when allocating for size
     * bits.
     * @param size: number of bits
     * @return
     */

    static auto overhead(uint size) -> size_t;

    static auto bytes(uint size) -> size_t;
    // modifiers
    /**
     * Change value of place'th bit
     * @param place: index of bit to change
     * @param value: desired value
     */

    inline void set(const uint place, const bool value) {
        assert(place < _nbytes * 8);
        uint offset = place % 8;
        uint theplace = place / 8;
        if (value) {
            raw()[theplace] |= _masks[offset];
        } else {
            raw()[theplace] &= ~_masks[offset];
        }
    }

    /**
     * Sets all memory on heap to 0
     */

    inline void zero() {
        if (_nbytes > 0 && _blob != nullptr) {
            memset(raw(), 0x0, _nbytes);
        }
    }

    /**
     * Deallocates memory stored on heap
     */

    inline void release() {
        if (_nbytes > BW_BSIZE)
            dealloc(_blob);
        _blob = nullptr;
        _nbytes = 0;
    }

    /**
     * Nice access to single bits
     * @param i: index to access
     * @return
     */

    inline auto operator[](unsigned int i) const -> uchar {
        if (i >= _nbytes) {
            return 0x0;
        }
        return const_raw()[i];
    }

    /**
     * Compares two wrappers. Assumes that smaller number of bytes also means
     * a smaller wrapper. Otherwise compares byte by byte.
     * @param other: wrapper to compare to
     * @return -1 if other is smaller, 0 if same, 1 if other is larger
     */
    [[nodiscard]] inline auto cmp(const BinaryWrapper &other) const -> int {
        if (_nbytes < other._nbytes)
            return -1;
        else if (_nbytes > other._nbytes)
            return 1;

        size_t bcmp = std::min(_nbytes, other._nbytes);
        return memcmp(const_raw(), other.const_raw(), bcmp);
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator==(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return enc1.cmp(enc2) == 0;
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator<(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return enc1.cmp(enc2) < 0;
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator!=(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return !(enc1 == enc2);
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator>=(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return !(enc1 < enc2);
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator>(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return enc2 < enc1;
    }

    /**
     * If sizes differs, the comparison is done here.
     * If sizes match, compares byte by byte.
     * @param enc1
     * @param enc2
     * @return true if a match, false otherwise
     */
    friend auto operator<=(const BinaryWrapper &enc1, const BinaryWrapper &enc2) -> bool {
        return enc2 < enc1;
    }

    const static uchar _masks[8];

  private:
    static inline auto allocate(size_t n) -> uchar * {
        if (n <= BW_BSIZE)
            return nullptr;
#ifndef NDEBUG
        size_t on = n;
#endif
        if (n % BW_BSIZE != 0)
            n = (1 + (n / BW_BSIZE)) * (BW_BSIZE);
        assert(n % BW_BSIZE == 0);
        assert(on <= n);
        return (uchar *)malloc(n);
    }

    static inline auto zallocate(size_t n) -> uchar * {
        if (n <= BW_BSIZE)
            return nullptr;
#ifndef NDEBUG
        size_t on = n;
#endif
        if (n % BW_BSIZE != 0) {
            n = (1 + (n / BW_BSIZE)) * (BW_BSIZE);
            assert(n == on + (BW_BSIZE - (on % BW_BSIZE)));
        }
        assert(n % BW_BSIZE == 0);
        assert(on <= n);
        return (uchar *)calloc(n, 1);
    }

    static inline void dealloc(uchar *data) { free(data); }

    static inline auto offset(uchar *data, uint16_t size) -> uchar * {
        //            if((size % __BW_BSIZE_) == 0) return data;
        //            else return &data[(__BW_BSIZE_ - (size % __BW_BSIZE_))];
        return data;
    }

    // blob of heap-allocated data
    uchar *_blob = nullptr;

    // number of bytes allocated on heap
    size_t _nbytes = 0;

    // masks for single-bit access
} __attribute__((packed));
} // namespace ptrie
namespace std {
auto operator<<(std::ostream &os, const ptrie::BinaryWrapper &b) -> std::ostream &;
}
#endif /* BINARYWRAPPER_H */
