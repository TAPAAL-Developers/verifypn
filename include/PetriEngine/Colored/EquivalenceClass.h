#ifndef EQUIVALENCECLASS_H
#define EQUIVALENCECLASS_H

#include "Intervals.h"
#include "Colors.h"
#include "ArcIntervals.h"

namespace PetriEngine {
    namespace Colored {
        class EquivalenceClass {
            public:
                EquivalenceClass(uint32_t id);
                EquivalenceClass(uint32_t id, const ColorType *colorType);
                EquivalenceClass(uint32_t id, const ColorType *colorType, interval_vector_t&& colorIntervals);
                ~EquivalenceClass() {}
                std::string to_string() const{
                    return _colorIntervals.to_string();
                }

                bool is_empty() const{
                    if(_colorIntervals.size() < 1 || _colorIntervals.front().size() < 1){
                        return true;
                    } 
                    return false;
                }

                bool contains_color(const std::vector<uint32_t> &ids, const std::vector<bool> &diagonalPositions) const;

                size_t size() const;

                EquivalenceClass intersect(uint32_t id, const EquivalenceClass &other) const;

                EquivalenceClass subtract(uint32_t id, const EquivalenceClass &other, const std::vector<bool> &diagonalPositions) const;
                
                uint32_t id() const { return _id; }
                const ColorType* type() const { return _colorType; }
                const interval_vector_t& intervals() const { return _colorIntervals; }
                void clear() { _colorIntervals.clear(); }
                void set_interval_vector(const interval_vector_t& interval) { _colorIntervals = interval; }
                void add_interval(interval_t&& interval) { _colorIntervals.add_interval(interval); }
            private:
                uint32_t _id;
                const ColorType *_colorType;
                interval_vector_t _colorIntervals;

            
        };
    }
}

#endif /* EQUIVALENCECLASS_H */