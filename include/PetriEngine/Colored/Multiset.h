/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   Multiset.h
 * Author: andreas
 *
 * Created on February 20, 2018, 10:37 AM
 */

#ifndef MULTISET_H
#define MULTISET_H

#include <utility>
#include <vector>

#include "Colors.h"

namespace PetriEngine::Colored {
class Multiset {
  private:
    class Iterator {
      private:
        const Multiset *_ms;
        size_t _index;

      public:
        Iterator(const Multiset *ms, size_t index) : _ms(ms), _index(index) {}

        auto operator==(Iterator &other) -> bool;
        auto operator!=(Iterator &other) -> bool;
        auto operator++() -> Iterator &;
        auto operator++(int) -> std::pair<const Color *, const uint32_t &>;
        auto operator*() -> std::pair<const Color *, const uint32_t &>;
    };

    using Internal = std::vector<std::pair<uint32_t, uint32_t>>;

  public:
    Multiset();
    Multiset(const Multiset &orig);
    Multiset(std::pair<const Color *, uint32_t> color);
    Multiset(std::vector<std::pair<const Color *, uint32_t>> &colors);
    virtual ~Multiset();

    auto operator+(const Multiset &other) const -> Multiset;
    auto operator-(const Multiset &other) const -> Multiset;
    auto operator*(uint32_t scalar) const -> Multiset;
    void operator+=(const Multiset &other);
    void operator-=(const Multiset &other);
    void operator*=(uint32_t scalar);
    auto operator[](const Color *color) const -> uint32_t;
    auto operator[](const Color *color) -> uint32_t &;

    [[nodiscard]] auto empty() const -> bool;
    void clean();

    [[nodiscard]] auto distinct_size() const -> size_t { return _set.size(); }

    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto begin() const -> const Iterator;
    [[nodiscard]] auto end() const -> const Iterator;

    [[nodiscard]] auto to_string() const -> std::string;

  private:
    Internal _set;
    const ColorType *_type;
};
} // namespace PetriEngine::Colored

#endif /* MULTISET_H */
