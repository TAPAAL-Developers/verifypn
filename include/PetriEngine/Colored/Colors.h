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

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <string.h>
#include <utility>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <cassert>

#include "Intervals.h"

namespace PetriEngine {
    namespace Colored {
        class ColorType;
        class Variable;
        class Color;

        typedef std::unordered_map<std::string, const ColorType*> ColorTypeMap;
        typedef std::unordered_map<const Variable *, const Color *> BindingMap;

        class Color final {
        public:
            friend std::ostream& operator<< (std::ostream& stream, const Color& color);

        protected:
            const std::vector<const Color*> _tuple;
            const ColorType * const _colorType;
            std::string _colorName;
            uint32_t _id;

        public:
            Color(const ColorType* colorType, uint32_t id, std::vector<const Color*>& colors);
            Color(const ColorType* colorType, uint32_t id, const char* color);
            ~Color() {}

            bool is_tuple() const {
                return _tuple.size() > 1;
            }

            void get_color_constraints(Colored::interval_t& constraintsVector, uint32_t& index) const;

            const std::vector<const Color*>& get_tuple_colors() const {
                return _tuple;
            }

            void get_tuple_id(std::vector<uint32_t>& idVector) const;

            const std::string& get_color_name() const {
                if (this->is_tuple()) {
                    throw "Cannot get color from a tuple color.";
                }
                return _colorName;
            }

            const ColorType* get_color_type() const {
                return _colorType;
            }

            uint32_t get_id() const {
                return _id;
            }

            const Color* operator[] (size_t index) const;
            bool operator< (const Color& other) const;
            bool operator> (const Color& other) const;
            bool operator<= (const Color& other) const;
            bool operator>= (const Color& other) const;

            bool operator== (const Color& other) const {
                return _colorType == other._colorType && _id == other._id;
            }
            bool operator!= (const Color& other) const {
                return !((*this) == other);
            }

            const Color& operator++ () const;
            const Color& operator-- () const;

            std::string to_string() const;
            static std::string to_string(const Color* color);
            static std::string to_string(const std::vector<const Color*>& colors);
        };

        class ColorType {
        private:
            std::vector<Color> _colors;
            std::string _name;
        public:

            ColorType(std::string name = "Undefined") : _colors(), _name(std::move(name)) {
            }

            virtual ~ColorType() = default;

            static const ColorType* dot_instance();
            virtual void add_color(const char* colorName);

            virtual size_t size() const {
                return _colors.size();
            }

            virtual size_t size(const std::vector<bool> &excludedFields) const {
                return _colors.size();
            }

            virtual size_t product_size() const {
                return 1;
            }

            virtual std::vector<size_t> get_constituents_sizes() const{
                std::vector<size_t> result;
                result.push_back(_colors.size());

                return result;
            }

            virtual Colored::interval_t get_full_interval() const{
                Colored::interval_t interval;
                interval.add_range(0, size()-1);
                return interval;
            }

            virtual void get_colortypes(std::vector<const ColorType *> &colorTypes) const{
                colorTypes.emplace_back(this);
            }

            virtual const Color& operator[] (size_t index) const {
                return _colors[index];
            }

            virtual const Color& operator[] (int index) const {
                return _colors[index];
            }

            virtual const Color& operator[] (uint32_t index) const {
                assert(index < _colors.size());
                return _colors[index];
            }

            virtual const Color* operator[] (const char* index) const;

            virtual const Color* operator[] (const std::string& index) const {
                return (*this)[index.c_str()];
            }

            virtual const Color* get_color(const std::vector<uint32_t> &ids) const {
                assert(ids.size() == 1);
                return &_colors[ids[0]];
            }

            const std::string& get_name() const {
                return _name;
            }

            std::vector<Color>::const_iterator begin() const {
                return _colors.begin();
            }

            std::vector<Color>::const_iterator end() const {
                return _colors.end();
            }
        };

        class ProductType : public ColorType {
        private:
            std::vector<const ColorType*> _constituents;
            mutable std::unordered_map<size_t,Color> _cache;

        public:
            ProductType(const std::string& name = "Undefined") : ColorType(name) {}
            ~ProductType() {
                _cache.clear();
            }

            void add_type(const ColorType* type) {
                _constituents.push_back(type);
            }

            void add_color(const char* colorName) override {}

            size_t size() const override {
                size_t product = 1;
                for (auto* ct : _constituents) {
                    product *= ct->size();
                }
                return product;
            }

            size_t size(const std::vector<bool> &excludedFields) const override {
                size_t product = 1;
                for (uint32_t i = 0; i < _constituents.size(); i++) {
                    if(!excludedFields[i]){
                        product *= _constituents[i]->size();
                    }
                }
                return product;
            }

            virtual size_t product_size() const{
                size_t size = 0;
                for (auto* ct : _constituents){
                    size += ct->product_size();
                }
                return size;
            }

            std::vector<size_t> get_constituents_sizes() const override{
                std::vector<size_t> result;
                for (auto* ct : _constituents) {
                    result.push_back(ct->size());
                }
                return result;
            }

            Colored::interval_t get_full_interval() const override{
                Colored::interval_t interval;
                for(auto ct : _constituents) {
                    interval.add_range(Reachability::range_t(0, ct->size()-1));
                }
                return interval;
            }

            void get_colortypes(std::vector<const ColorType *> &colorTypes) const override{
                for(auto ct : _constituents){
                    ct->get_colortypes(colorTypes);
                }
            }

            bool contains_types(const std::vector<const ColorType*>& types) const {
                if (_constituents.size() != types.size()) return false;

                for (size_t i = 0; i < _constituents.size(); ++i) {
                    if (!(_constituents[i] == types[i])) {
                        return false;
                    }
                }

                return true;
            }

            const ColorType* get_nested_color_type(size_t index) const {
                return _constituents[index];
            }

            const Color* get_color(const std::vector<uint32_t> &ids) const override;

            const Color* get_color(const std::vector<const Color*>& colors) const;

            const Color& operator[](size_t index) const override;
            const Color& operator[](int index) const override {
                return operator[]((size_t)index);
            }
            const Color& operator[](uint32_t index) const override {
                return operator[]((size_t)index);
            }

            const Color* operator[](const char* index) const override;
            const Color* operator[](const std::string& index) const override;
        };

        struct Variable {
            std::string _name;
            const ColorType* _colorType;
        };

        struct ColorFixpoint {
            Colored::interval_vector_t _constraints;
            bool _in_queue;
        };

        struct ColorTypePartition {
            std::vector<const Color *> _colors;
            std::string _name;
        };

        typedef std::unordered_map<uint32_t, const Colored::Variable *> PositionVariableMap;
        //Map from variables to a vector of maps from variable positions to the modifiers applied to the variable in that position
        typedef std::unordered_map<const Colored::Variable *,std::vector<std::unordered_map<uint32_t, int32_t>>> VariableModifierMap;
        typedef std::unordered_map<const PetriEngine::Colored::Variable *, PetriEngine::Colored::interval_vector_t> VariableIntervalMap;
        typedef std::unordered_map<uint32_t, std::vector<const Color*>> PositionColorsMap;
    }
}

#endif /* COLORS_H */

