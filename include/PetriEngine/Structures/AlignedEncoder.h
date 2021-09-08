/*
 * File:   Encoder.h
 * Author: Peter G. Jensen
 *
 * Created on 11 March 2016, 14:15
 */

#ifndef ALIGNEDENCODER_H
#define ALIGNEDENCODER_H

#include "binarywrapper.h"
#include <cmath>

using namespace ptrie;

class AlignedEncoder {
    using scratchpad_t = BinaryWrapper;

  public:
    AlignedEncoder(uint32_t places);

    ~AlignedEncoder();

    auto encode(const uint32_t *data, unsigned char type) -> size_t;

    void decode(uint32_t *destination, const unsigned char *source);

    auto scratchpad() -> BinaryWrapper & { return _scratchpad; }

    [[nodiscard]] auto get_type(uint32_t sum, uint32_t pwt, bool same, uint32_t val) const
        -> unsigned char;

    auto size(const uchar *data) const -> size_t;

  private:
    [[nodiscard]] auto token_bytes(uint32_t ntokens) const -> uint32_t;

    auto write_bit_vector(const uint32_t *data) -> uint32_t;

    auto write_two_bit_vector(const uint32_t *data) -> uint32_t;

    template <typename T> auto write_tokens(const uint32_t *data) -> uint32_t;

    template <typename T> auto write_token_counts(size_t offset, const uint32_t *data) -> uint32_t;

    auto write_places(const uint32_t *data) -> uint32_t;

    auto read_bit_vector(uint32_t *destination, const unsigned char *source, uint32_t value)
        -> uint32_t;

    auto read_two_bit_vector(uint32_t *destination, const unsigned char *source) -> uint32_t;

    auto read_places(uint32_t *destination, const unsigned char *source, uint32_t offset,
                     uint32_t value) -> uint32_t;

    template <typename T>
    auto read_tokens(uint32_t *destination, const unsigned char *source) -> uint32_t;

    template <typename T>
    auto read_place_token_counts(uint32_t *destination, const unsigned char *source,
                                 uint32_t offset) const -> uint32_t;

    template <typename T>
    auto place_token_counts_size(const unsigned char *source, uint32_t offset) const -> size_t;

    template <typename T>
    auto read_bit_token_counts(uint32_t *destination, const unsigned char *source) const
        -> uint32_t;

    template <typename T> auto bit_token_counts_size(const unsigned char *source) const -> size_t;

    uint32_t _places;

    uint32_t _psize;

    // dummy value for template
    scratchpad_t _scratchpad;
};

#endif /* ALIGNEDENCODER_H */
