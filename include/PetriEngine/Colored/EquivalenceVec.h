#ifndef EQUIVALENCEVEC_H
#define EQUIVALENCEVEC_H

#include "ArcIntervals.h"
#include "Colors.h"
#include "EquivalenceClass.h"
#include "Intervals.h"

namespace PetriEngine::Colored {
class EquivalenceVec {
  public:
    void apply_partition(Colored::arc_intervals_t &arcInterval) const;
    void merge_eq_classes();
    void apply_partition(std::vector<uint32_t> &colorIds) const;

    auto is_diagonal() const -> bool { return _diagonal; }

    void set_diagonal(bool diagonal) { _diagonal = diagonal; }

    auto get_eq_classes() const -> const std::vector<EquivalenceClass> & {
        return _equivalenceClasses;
    }

    auto get_dagonal_tuple_positions() const -> const std::vector<bool> & {
        return _diagonalTuplePositions;
    }

    void push_back_eq_class(const EquivalenceClass &Eqclass) {
        _equivalenceClasses.push_back(Eqclass);
    }

    void erase_eq_class(uint32_t position) {
        _equivalenceClasses.erase(_equivalenceClasses.begin() + position);
    }

    void push_back_diagonal_tuple_pos(bool val) { _diagonalTuplePositions.push_back(val); }

    void add_color_to_eq_class_map(const Color *color);

    void set_diagonal_tuple_position(uint32_t position, bool value) {
        _diagonalTuplePositions[position] = value;
    }

    void set_diagonal_tuple_positions(const std::vector<bool> &diagonalPositions) {
        _diagonalTuplePositions = diagonalPositions;
    }

    auto get_color_eq_class_map() const
        -> const std::unordered_map<const Colored::Color *, EquivalenceClass *> & {
        return _colorEQClassMap;
    }

  private:
    std::vector<EquivalenceClass> _equivalenceClasses;
    std::unordered_map<const Colored::Color *, EquivalenceClass *> _colorEQClassMap;
    std::vector<bool> _diagonalTuplePositions;
    bool _diagonal = false;
};
} // namespace PetriEngine::Colored

#endif /* EQUIVALENCEVEC_H */