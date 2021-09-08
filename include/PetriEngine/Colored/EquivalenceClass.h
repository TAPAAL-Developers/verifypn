#ifndef EQUIVALENCECLASS_H
#define EQUIVALENCECLASS_H

#include "ArcIntervals.h"
#include "Colors.h"
#include "Intervals.h"

namespace PetriEngine::Colored {
class EquivalenceClass {
  public:
    EquivalenceClass(uint32_t id);
    EquivalenceClass(uint32_t id, const ColorType *colorType);
    EquivalenceClass(uint32_t id, const ColorType *colorType, IntervalVector &&colorIntervals);
    ~EquivalenceClass() = default;
    [[nodiscard]] auto to_string() const -> std::string { return _colorIntervals.to_string(); }

    [[nodiscard]] auto is_empty() const -> bool {
        if (_colorIntervals.size() < 1 || _colorIntervals.front().size() < 1) {
            return true;
        }
        return false;
    }

    [[nodiscard]] auto contains_color(const std::vector<uint32_t> &ids,
                                      const std::vector<bool> &diagonalPositions) const -> bool;

    [[nodiscard]] auto size() const -> size_t;

    [[nodiscard]] auto intersect(uint32_t id, const EquivalenceClass &other) const
        -> EquivalenceClass;

    [[nodiscard]] auto subtract(uint32_t id, const EquivalenceClass &other,
                                const std::vector<bool> &diagonalPositions) const
        -> EquivalenceClass;

    [[nodiscard]] auto id() const -> uint32_t { return _id; }
    [[nodiscard]] auto type() const -> const ColorType * { return _colorType; }
    [[nodiscard]] auto intervals() const -> const IntervalVector & { return _colorIntervals; }
    void clear() { _colorIntervals.clear(); }
    void set_interval_vector(const IntervalVector &interval) { _colorIntervals = interval; }
    void add_interval(interval_t &&interval) { _colorIntervals.add_interval(interval); }

  private:
    uint32_t _id;
    const ColorType *_colorType;
    IntervalVector _colorIntervals;
};
} // namespace PetriEngine::Colored

#endif /* EQUIVALENCECLASS_H */