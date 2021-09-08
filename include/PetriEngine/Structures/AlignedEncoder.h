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
    typedef binarywrapper_t scratchpad_t;

  public:
    AlignedEncoder(uint32_t places, uint32_t k);

    ~AlignedEncoder();

    size_t encode(const uint32_t *data, unsigned char type);

    void decode(uint32_t *destination, const unsigned char *source);

    binarywrapper_t &scratchpad() { return _scratchpad; }

    unsigned char get_type(uint32_t sum, uint32_t pwt, bool same, uint32_t val) const;

    size_t size(const uchar *data) const;

  private:
    uint32_t token_bytes(uint32_t ntokens) const;

    uint32_t write_bit_vector(size_t offset, const uint32_t *data);

    uint32_t write_two_bit_vector(size_t offset, const uint32_t *data);

    template <typename T> uint32_t write_tokens(size_t offset, const uint32_t *data);

    template <typename T> uint32_t write_token_counts(size_t offset, const uint32_t *data);

    uint32_t write_places(size_t offset, const uint32_t *data);

    uint32_t read_bit_vector(uint32_t *destination, const unsigned char *source, uint32_t offset,
                             uint32_t value);

    uint32_t read_two_bit_vector(uint32_t *destination, const unsigned char *source,
                                 uint32_t offset);

    uint32_t read_places(uint32_t *destination, const unsigned char *source, uint32_t offset,
                         uint32_t value);

    template <typename T>
    uint32_t read_tokens(uint32_t *destination, const unsigned char *source, uint32_t offset);

    template <typename T>
    uint32_t read_place_token_counts(uint32_t *destination, const unsigned char *source,
                                     uint32_t offset) const;

    template <typename T>
    size_t place_token_counts_size(const unsigned char *source, uint32_t offset) const;

    template <typename T>
    uint32_t read_bit_token_counts(uint32_t *destination, const unsigned char *source,
                                   uint32_t offset) const;

    template <typename T>
    size_t bit_token_counts_size(const unsigned char *source, uint32_t offset) const;

    uint32_t _places;

    uint32_t _psize;

    // dummy value for template
    scratchpad_t _scratchpad;
};

#endif /* ALIGNEDENCODER_H */
