/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLORS_H
#define COLORS_H

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Intervals.h"

namespace PetriEngine::Colored {
class ColorType;
class Variable;
class Color;

using ColorTypeMap = std::unordered_map<std::string, const ColorType *>;
using BindingMap = std::unordered_map<const Variable *, const Color *>;

class Color final {
  public:
    friend auto operator<<(std::ostream &stream, const Color &color) -> std::ostream &;

  protected:
    const std::vector<const Color *> _tuple;
    const ColorType *const _colorType;
    std::string _colorName;
    uint32_t _id;

  public:
    Color(const ColorType *colorType, uint32_t id, std::vector<const Color *> &colors);
    Color(const ColorType *colorType, uint32_t id, const char *color);
    ~Color() = default;

    [[nodiscard]] auto is_tuple() const -> bool { return _tuple.size() > 1; }

    void get_color_constraints(Colored::interval_t &constraintsVector, uint32_t &index) const;

    [[nodiscard]] auto get_tuple_colors() const -> const std::vector<const Color *> & {
        return _tuple;
    }

    void get_tuple_id(std::vector<uint32_t> &idVector) const;

    [[nodiscard]] auto get_color_name() const -> const std::string & {
        if (this->is_tuple()) {
            throw "Cannot get color from a tuple color.";
        }
        return _colorName;
    }

    [[nodiscard]] auto get_color_type() const -> const ColorType * { return _colorType; }

    [[nodiscard]] auto get_id() const -> uint32_t { return _id; }

    auto operator[](size_t index) const -> const Color *;
    auto operator<(const Color &other) const -> bool;
    auto operator>(const Color &other) const -> bool;
    auto operator<=(const Color &other) const -> bool;
    auto operator>=(const Color &other) const -> bool;

    auto operator==(const Color &other) const -> bool {
        return _colorType == other._colorType && _id == other._id;
    }
    auto operator!=(const Color &other) const -> bool { return !((*this) == other); }

    auto operator++() const -> const Color &;
    auto operator--() const -> const Color &;

    [[nodiscard]] auto to_string() const -> std::string;
    static auto to_string(const Color *color) -> std::string;
    static auto to_string(const std::vector<const Color *> &colors) -> std::string;
};

class ColorType {
  private:
    std::vector<Color> _colors;
    std::string _name;

  public:
    ColorType(std::string name = "Undefined") : _colors(), _name(std::move(name)) {}

    virtual ~ColorType() = default;

    static auto dot_instance() -> const ColorType *;
    virtual void add_color(const char *colorName);

    [[nodiscard]] virtual auto size() const -> size_t { return _colors.size(); }

    [[nodiscard]] virtual auto size(const std::vector<bool> &excludedFields) const -> size_t {
        return _colors.size();
    }

    [[nodiscard]] virtual auto product_size() const -> size_t { return 1; }

    [[nodiscard]] virtual auto get_constituents_sizes() const -> std::vector<size_t> {
        std::vector<size_t> result;
        result.push_back(_colors.size());

        return result;
    }

    [[nodiscard]] virtual auto get_full_interval() const -> Colored::interval_t {
        Colored::interval_t interval;
        interval.add_range(0, size() - 1);
        return interval;
    }

    virtual void get_colortypes(std::vector<const ColorType *> &colorTypes) const {
        colorTypes.emplace_back(this);
    }

    virtual auto operator[](size_t index) const -> const Color & { return _colors[index]; }

    virtual auto operator[](int index) const -> const Color & { return _colors[index]; }

    virtual auto operator[](uint32_t index) const -> const Color & {
        assert(index < _colors.size());
        return _colors[index];
    }

    virtual auto operator[](const char *index) const -> const Color *;

    virtual auto operator[](const std::string &index) const -> const Color * {
        return (*this)[index.c_str()];
    }

    [[nodiscard]] virtual auto get_color(const std::vector<uint32_t> &ids) const -> const Color * {
        assert(ids.size() == 1);
        return &_colors[ids[0]];
    }

    [[nodiscard]] auto get_name() const -> const std::string & { return _name; }

    [[nodiscard]] auto begin() const -> std::vector<Color>::const_iterator {
        return _colors.begin();
    }

    [[nodiscard]] auto end() const -> std::vector<Color>::const_iterator { return _colors.end(); }
};

class ProductType : public ColorType {
  private:
    std::vector<const ColorType *> _constituents;
    mutable std::unordered_map<size_t, Color> _cache;

  public:
    ProductType(const std::string &name = "Undefined") : ColorType(name) {}
    ~ProductType() override { _cache.clear(); }

    void add_type(const ColorType *type) { _constituents.push_back(type); }

    void add_color(const char *colorName) override {}

    auto size() const -> size_t override {
        size_t product = 1;
        for (auto *ct : _constituents) {
            product *= ct->size();
        }
        return product;
    }

    auto size(const std::vector<bool> &excludedFields) const -> size_t override {
        size_t product = 1;
        for (uint32_t i = 0; i < _constituents.size(); i++) {
            if (!excludedFields[i]) {
                product *= _constituents[i]->size();
            }
        }
        return product;
    }

    auto product_size() const -> size_t override {
        size_t size = 0;
        for (auto *ct : _constituents) {
            size += ct->product_size();
        }
        return size;
    }

    auto get_constituents_sizes() const -> std::vector<size_t> override {
        std::vector<size_t> result;
        for (auto *ct : _constituents) {
            result.push_back(ct->size());
        }
        return result;
    }

    auto get_full_interval() const -> Colored::interval_t override {
        Colored::interval_t interval;
        for (auto ct : _constituents) {
            interval.add_range(Reachability::range_t(0, ct->size() - 1));
        }
        return interval;
    }

    void get_colortypes(std::vector<const ColorType *> &colorTypes) const override {
        for (auto ct : _constituents) {
            ct->get_colortypes(colorTypes);
        }
    }

    auto contains_types(const std::vector<const ColorType *> &types) const -> bool {
        if (_constituents.size() != types.size())
            return false;

        for (size_t i = 0; i < _constituents.size(); ++i) {
            if (_constituents[i] != types[i]) {
                return false;
            }
        }

        return true;
    }

    auto get_nested_color_type(size_t index) const -> const ColorType * {
        return _constituents[index];
    }

    auto get_color(const std::vector<uint32_t> &ids) const -> const Color * override;

    auto get_color(const std::vector<const Color *> &colors) const -> const Color *;

    auto operator[](size_t index) const -> const Color & override;
    auto operator[](int index) const -> const Color & override { return operator[]((size_t)index); }
    auto operator[](uint32_t index) const -> const Color & override {
        return operator[]((size_t)index);
    }

    auto operator[](const char *index) const -> const Color * override;
    auto operator[](const std::string &index) const -> const Color * override;
};

struct Variable {
    std::string _name;
    const ColorType *_colorType;
};

struct color_fixpoint_t {
    Colored::IntervalVector _constraints;
    bool _in_queue;
};

struct color_type_partition_t {
    std::vector<const Color *> _colors;
    std::string _name;
};

using PositionVariableMap = std::unordered_map<uint32_t, const Colored::Variable *>;
// Map from variables to a vector of maps from variable positions to the modifiers applied to the
// variable in that position
using VariableModifierMap = std::unordered_map<const Colored::Variable *,
                                               std::vector<std::unordered_map<uint32_t, int32_t>>>;
using VariableIntervalMap = std::unordered_map<const PetriEngine::Colored::Variable *,
                                               PetriEngine::Colored::IntervalVector>;
using PositionColorsMap = std::unordered_map<uint32_t, std::vector<const Color *>>;
} // namespace PetriEngine::Colored

#endif /* COLORS_H */
