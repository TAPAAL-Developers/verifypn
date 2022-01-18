#ifndef EQUIVALENCEVEC_H
#define EQUIVALENCEVEC_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"
#include "EquivalenceClass.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceVec{
            public:
                using eq_vector_t = std::vector<EquivalenceClass>;
                void applyPartition(Colored::ArcIntervals& arcInterval) const;
                void mergeEqClasses();
                void applyPartition(std::vector<uint32_t> &colorIds) const;

                bool isDiagonal() const{
                    return _diagonal;
                }

                void setDiagonal(bool diagonal) {
                    _diagonal = diagonal;
                }

                const uint32_t getUniqueIdForColor(const Colored::Color *color) const;


                const std::vector<bool> & getDiagonalTuplePositions() const{
                    return _diagonalTuplePositions;
                }

                void push_back(const EquivalenceClass &Eqclass){
                    _equivalenceClasses.push_back(Eqclass);
                }

                void erase(uint32_t position){
                    _equivalenceClasses.erase(_equivalenceClasses.begin() + position);
                }

                void push_back_diagonalTuplePos(bool val){
                    _diagonalTuplePositions.push_back(val);
                }

                void addColorToEqClassMap(const Color *color);

                void setDiagonalTuplePosition(uint32_t position, bool value){
                    _diagonalTuplePositions[position] = value;
                }

                void setDiagonalTuplePositions(const std::vector<bool> &diagonalPositions){
                    _diagonalTuplePositions = diagonalPositions;
                }

                const std::unordered_map<const Colored::Color *, size_t> &getColorEqClassMap() const{
                    return _colorEQClassMap;
                }

                bool empty() const {
                    return _equivalenceClasses.empty();
                }

                size_t size() const {
                    return _equivalenceClasses.size();
                }

                const EquivalenceClass& back() const {
                    return _equivalenceClasses.back();
                }

                const EquivalenceClass& operator[](size_t i) const {
                    return _equivalenceClasses[i];
                }

                eq_vector_t::const_iterator begin() const { return _equivalenceClasses.begin(); }
                eq_vector_t::const_iterator end() const { return _equivalenceClasses.end(); }

                eq_vector_t::iterator begin() { return _equivalenceClasses.begin(); }
                eq_vector_t::iterator end() { return _equivalenceClasses.end(); }

                size_t class_id(const Colored::Color* color) const {
                    return _equivalenceClasses[_colorEQClassMap.find(color)->second].id();
                }

                const eq_vector_t& get_classes() const {
                    return _equivalenceClasses;
                }

            private:
                eq_vector_t _equivalenceClasses;
                std::unordered_map<const Colored::Color *, size_t> _colorEQClassMap;
                std::vector<bool> _diagonalTuplePositions;
                bool _diagonal = false;
        };
    }
}

#endif /* EQUIVALENCEVEC_H */