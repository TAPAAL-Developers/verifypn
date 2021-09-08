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

#include "PetriEngine/Colored/Colors.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

//@{
// From: https://stackoverflow.com/a/236803
template <typename Out> void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

auto split(const std::string &s, char delim) -> std::vector<std::string> {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}
//}@

namespace PetriEngine::Colored {
/*std::ostream& operator<<(std::ostream& stream, const Color& color) {
    stream << color.to_string();
    return stream;
}*/

Color::Color(const ColorType *colorType, uint32_t id, std::vector<const Color *> &colors)
    : _tuple(colors), _colorType(colorType), _colorName(""), _id(id) {
    if (colorType != nullptr)
        assert(id <= colorType->size());
}

Color::Color(const ColorType *colorType, uint32_t id, const char *color)
    : _tuple(), _colorType(colorType), _colorName(color), _id(id) {
    if (colorType != nullptr)
        assert(id <= colorType->size());
}

auto Color::operator++() const -> const Color & {
    // std::cout << _colorName <<" " << _colorType->get_name() << std::endl;
    if (_id >= _colorType->size() - 1) {
        // std::cout << "inside if" << std::endl;
        return (*_colorType)[0];
    }
    assert(_id + 1 < _colorType->size());
    return (*_colorType)[_id + 1];
}

auto Color::operator--() const -> const Color & {
    if (_id <= 0) {
        return (*_colorType)[_colorType->size() - 1];
    }
    assert(_id - 1 >= 0);
    return (*_colorType)[_id - 1];
}

auto Color::to_string() const -> std::string { return to_string(this); }

void Color::get_color_constraints(Colored::interval_t &constraintsVector, uint32_t &index) const {
    if (this->is_tuple()) {
        for (const Color *color : _tuple) {
            color->get_color_constraints(constraintsVector, index);
            index++;
        }
    } else {
        Reachability::range_t curRange;
        if (index >= constraintsVector.size()) {
            curRange &= _id;
            constraintsVector.add_range(curRange);
        } else {
            curRange = constraintsVector[index];
            if (_id < curRange._lower) {
                curRange._lower = _id;
            }
            if (_id > curRange._upper) {
                curRange._upper = _id;
            }

            constraintsVector[index] = curRange;
        }
    }
}

void Color::get_tuple_id(std::vector<uint32_t> &idVector) const {
    if (this->is_tuple()) {
        for (auto *color : _tuple) {
            color->get_tuple_id(idVector);
        }
    } else {
        idVector.push_back(_id);
    }
}

auto Color::to_string(const Color *color) -> std::string {
    if (color->is_tuple()) {
        std::ostringstream oss;
        oss << "(";
        for (size_t i = 0; i < color->_tuple.size(); i++) {
            oss << color->_tuple[i]->to_string();
            if (i < color->_tuple.size() - 1)
                oss << ",";
        }
        oss << ")";
        return oss.str();
    }
    return std::string(color->_colorName);
}

auto Color::to_string(const std::vector<const Color *> &colors) -> std::string {
    std::ostringstream oss;
    if (colors.size() > 1)
        oss << "(";

    for (size_t i = 0; i < colors.size(); i++) {
        oss << colors[i]->to_string();
        if (i < colors.size() - 1)
            oss << ",";
    }

    if (colors.size() > 1)
        oss << ")";
    return oss.str();
}

auto ColorType::dot_instance() -> const ColorType * {
    static ColorType instance("dot");
    if (instance.size() == 0) {
        instance.add_color("dot");
    }
    return &instance;
}

void ColorType::add_color(const char *colorName) {
    _colors.emplace_back(this, _colors.size(), colorName);
}

auto ColorType::operator[](const char *index) const -> const Color * {
    for (size_t i = 0; i < _colors.size(); i++) {
        if (strcmp(operator[](i).to_string().c_str(), index) == 0)
            return &operator[](i);
    }
    return nullptr;
}

auto ProductType::operator[](size_t index) const -> const Color & {
    if (_cache.count(index) < 1) {
        size_t mod = 1;
        size_t div = 1;

        std::vector<const Color *> colors;
        for (auto &constituent : _constituents) {
            mod = constituent->size();
            colors.push_back(&(*constituent)[(index / div) % mod]);
            div *= mod;
        }

        _cache.emplace(index, Color(this, index, colors));
    }

    return _cache.at(index);
}

auto ProductType::get_color(const std::vector<const Color *> &colors) const -> const Color * {
    size_t product = 1;
    size_t sum = 0;

    if (_constituents.size() != colors.size())
        return nullptr;

    for (size_t i = 0; i < _constituents.size(); ++i) {
        if (!(colors[i]->get_color_type() == _constituents[i]))
            return nullptr;

        sum += product * colors[i]->get_id();
        product *= _constituents[i]->size();
    }
    return &operator[](sum);
}

auto ProductType::get_color(const std::vector<uint32_t> &ids) const -> const Color * {
    assert(ids.size() == _constituents.size());
    size_t product = 1;
    size_t sum = 0;

    for (size_t i = 0; i < _constituents.size(); ++i) {
        sum += product * ids[i];
        product *= _constituents[i]->size();
    }
    return &operator[](sum);
}

auto ProductType::operator[](const char *index) const -> const Color * {
    return operator[](std::string(index));
}

auto ProductType::operator[](const std::string &index) const -> const Color * {
    std::string str(index.substr(1, index.size() - 2));
    std::vector<std::string> parts = split(str, ',');

    if (parts.size() != _constituents.size()) {
        return nullptr;
    }

    size_t sum = 0;
    size_t mult = 1;
    for (size_t i = 0; i < parts.size(); ++i) {
        sum += mult * (*_constituents[i])[parts[i]]->get_id();
        mult *= _constituents[i]->size();
    }

    return &operator[](sum);
}

} // namespace PetriEngine::Colored
