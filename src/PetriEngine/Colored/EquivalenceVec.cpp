
#include "PetriEngine/Colored/EquivalenceVec.h"

namespace PetriEngine::Colored {
void EquivalenceVec::apply_partition(Colored::ArcIntervals &arcInterval) const {
    if (_diagonal || _equivalenceClasses.size() >=
                         _equivalenceClasses.back().type()->size(_diagonalTuplePositions)) {
        return;
    }
    std::vector<Colored::interval_vector_t> newTupleVec;
    for (auto &intervalTuple : arcInterval._intervalTupleVec) {
        intervalTuple.combine_neighbours();
        interval_vector_t newIntervalTuple;
        for (const auto &interval : intervalTuple) {
            for (const auto &EQClass : _equivalenceClasses) {
                for (const auto &EQinterval : EQClass.intervals()) {
                    auto overlap = interval.get_overlap(EQinterval, _diagonalTuplePositions);
                    if (overlap.is_sound()) {
                        auto singleInterval = EQinterval.get_single_color_interval();
                        for (uint32_t i = 0; i < _diagonalTuplePositions.size(); i++) {
                            if (_diagonalTuplePositions[i]) {
                                singleInterval[i] = interval[i];
                            }
                        }
                        newIntervalTuple.add_interval(singleInterval);
                        continue;
                    }
                }
            }
        }
        newTupleVec.push_back(std::move(newIntervalTuple));
    }
    arcInterval._intervalTupleVec = std::move(newTupleVec);
}

void EquivalenceVec::merge_eq_classes() {
    for (int32_t i = _equivalenceClasses.size() - 1; i >= 1; i--) {
        for (int32_t j = i - 1; j >= 0; j--) {
            bool fullyContained = true;
            for (const auto &interval : _equivalenceClasses[i].intervals()) {
                if (!_equivalenceClasses[j].intervals().contains(interval,
                                                                 _diagonalTuplePositions)) {
                    fullyContained = false;
                    break;
                }
            }
            if (fullyContained) {
                _equivalenceClasses.erase(_equivalenceClasses.begin() + i);
            }
        }
    }
}

void EquivalenceVec::add_color_to_eq_class_map(const Color *color) {
    for (auto &eqClass : _equivalenceClasses) {
        std::vector<uint32_t> colorIds;
        color->get_tuple_id(colorIds);
        if (eqClass.contains_color(colorIds, _diagonalTuplePositions)) {
            _colorEQClassMap[color] = &eqClass;
            break;
        }
    }
}

void EquivalenceVec::apply_partition(std::vector<uint32_t> &colorIds) const {
    if (_diagonal || _equivalenceClasses.size() >=
                         _equivalenceClasses.back().type()->size(_diagonalTuplePositions)) {
        return;
    }

    interval_t interval;
    for (auto colorId : colorIds) {
        interval.add_range(colorId, colorId);
    }

    for (const auto &EqClass : _equivalenceClasses) {
        for (const auto &EqInterval : EqClass.intervals()) {
            if (EqInterval.contains(interval, _diagonalTuplePositions)) {
                auto singleInterval = EqInterval.get_single_color_interval();
                for (uint32_t i = 0; i < singleInterval.size(); i++) {
                    colorIds[i] =
                        _diagonalTuplePositions[i] ? interval[i]._lower : singleInterval[i]._lower;
                }
            }
        }
    }
}
} // namespace PetriEngine::Colored