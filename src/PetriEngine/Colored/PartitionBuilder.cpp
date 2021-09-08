#include "PetriEngine/Colored/PartitionBuilder.h"
#include <chrono>
#include <numeric>

namespace PetriEngine::Colored {

PartitionBuilder::PartitionBuilder(
    const std::vector<Transition> &transitions, const std::vector<Place> &places,
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePostTransitionMap,
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePreTransitionMap)
    : PartitionBuilder(transitions, places, placePostTransitionMap, placePreTransitionMap,
                       nullptr) {}

PartitionBuilder::PartitionBuilder(
    const std::vector<Transition> &transitions, const std::vector<Place> &places,
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePostTransitionMap,
    const std::unordered_map<uint32_t, std::vector<uint32_t>> &placePreTransitionMap,
    const std::vector<Colored::ColorFixpoint> *placeColorFixpoints)
    : _transitions(transitions), _places(places), _placePostTransitionMap(placePostTransitionMap),
      _placePreTransitionMap(placePreTransitionMap) {

    // Instantiate partitions
    for (uint32_t i = 0; i < _places.size(); i++) {
        const PetriEngine::Colored::Place &place = _places[i];
        EquivalenceClass fullClass = EquivalenceClass(++_eq_id_counter, place._type);
        if (placeColorFixpoints != nullptr) {
            fullClass.set_interval_vector((*placeColorFixpoints)[i]._constraints);
        } else {
            fullClass.add_interval(place._type->get_full_interval());
        }
        _partition[i].push_back_eq_class(fullClass);
        for (uint32_t j = 0; j < place._type->product_size(); j++) {
            _partition[i].push_back_diagonal_tuple_pos(false);
        }
        _placeQueue.push_back(i);
        _inQueue[i] = true;
    }
}

void PartitionBuilder::print_partion() const {
    for (const auto &equivalenceVec : _partition) {
        std::cout << "Partition for place " << _places[equivalenceVec.first]._name << std::endl;
        std::cout << "Diag variables: (";
        for (auto daigPos : equivalenceVec.second.get_dagonal_tuple_positions()) {
            std::cout << daigPos << ",";
        }
        std::cout << ")" << std::endl;
        for (const auto &equivalenceClass : equivalenceVec.second.get_eq_classes()) {
            std::cout << equivalenceClass.to_string() << std::endl;
        }
        std::cout << "Diagonal " << equivalenceVec.second.is_diagonal() << std::endl << std::endl;
    }
}

auto PartitionBuilder::partition_net(int32_t timeout) -> bool {
    const auto start = std::chrono::high_resolution_clock::now();
    handle_leaf_transitions();
    auto end = std::chrono::high_resolution_clock::now();

    while (!_placeQueue.empty() && timeout > 0 &&
           std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < timeout) {
        auto placeId = _placeQueue.back();
        _placeQueue.pop_back();
        _inQueue[placeId] = false;

        bool allPositionsDiagonal = true;
        for (auto diag : _partition[placeId].get_dagonal_tuple_positions()) {
            if (!diag) {
                allPositionsDiagonal = false;
                break;
            }
        }

        if (allPositionsDiagonal || _partition[placeId].get_eq_classes().size() >=
                                        _partition[placeId].get_eq_classes().back().type()->size(
                                            _partition[placeId].get_dagonal_tuple_positions())) {
            _partition[placeId].set_diagonal(true);
        }
        if (_placePreTransitionMap.find(placeId) != _placePreTransitionMap.end()) {
            for (uint32_t transitionId : _placePreTransitionMap.find(placeId)->second) {
                handle_transition(transitionId, placeId);
            }
        }
        end = std::chrono::high_resolution_clock::now();
    }
    return _placeQueue.empty();
}

void PartitionBuilder::assign_color_map(
    std::unordered_map<uint32_t, EquivalenceVec> &partition) const {
    for (auto &eqVec : partition) {
        if (eqVec.second.is_diagonal()) {
            continue;
        }
        const ColorType *colorType = _places[eqVec.first]._type;
        for (const auto &i : *colorType) {
            const Color *color = &i;
            eqVec.second.add_color_to_eq_class_map(color);
        }
    }
}

void PartitionBuilder::handle_transition(uint32_t transitionId, uint32_t postPlaceId) {
    const PetriEngine::Colored::Transition &transition = _transitions[transitionId];
    Arc postArc;
    bool arcFound = false;
    for (const auto &outArc : transition._output_arcs) {
        if (outArc._place == postPlaceId) {
            postArc = outArc;
            arcFound = true;
            break;
        }
    }

    if (!arcFound) {
        return;
    }

    handle_transition(transition, postPlaceId, &postArc);
}

// Check if a variable appears more than once on the output arc
// If the place is does not have a product colortype mark the whole place as diagonal, otherwise
// only the positions
void PartitionBuilder::check_var_on_arc(const VariableModifierMap &varModifierMap,
                                        std::set<const Colored::Variable *> &diagonalVars,
                                        uint32_t placeId, bool inputArc) {
    for (const auto &varModMap : varModifierMap) {
        if (varModMap.second.size() > 1) {
            uint32_t actualSize = 0;
            std::vector<uint32_t> positions;
            for (const auto &map : varModMap.second) {
                if (!map.empty()) {
                    for (auto position : map) {
                        positions.push_back(position.first);
                    }
                    actualSize++;
                }
            }
            if (actualSize > 1) {
                diagonalVars.insert(varModMap.first);
                if (_partition[placeId].get_eq_classes().back().type()->product_size() == 1) {
                    _partition[placeId].set_diagonal(true);
                } else {
                    for (auto pos : positions) {
                        if (!_partition[placeId].get_dagonal_tuple_positions()[pos]) {
                            if (inputArc)
                                add_to_queue(placeId);
                            _partition[placeId].set_diagonal_tuple_position(pos, true);
                        }
                    }
                }
            }
        }
    }
}
// Check if the variales on this preArc also appear on other preArcs for the transition
// and mark them as diagonal if they do
void PartitionBuilder::check_var_on_input_arcs(
    const std::unordered_map<uint32_t, PositionVariableMap> &placeVariableMap,
    const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable *> &diagonalVars,
    uint32_t placeId) {
    for (const auto &placeVariables : placeVariableMap) {
        for (const auto &variable : preVarPositionMap) {
            for (const auto &varPosition : placeVariables.second) {
                if (varPosition.second == variable.second) {
                    diagonalVars.insert(variable.second);
                    if (_partition[placeId].get_eq_classes().back().type()->product_size() == 1) {
                        _partition[placeId].set_diagonal(true);
                    } else if (!_partition[placeId].get_dagonal_tuple_positions()[variable.first]) {
                        add_to_queue(placeId);
                        _partition[placeId].set_diagonal_tuple_position(variable.first, true);
                    }

                    if (_partition[placeVariables.first]
                            .get_eq_classes()
                            .back()
                            .type()
                            ->product_size() == 1) {
                        _partition[placeVariables.first].set_diagonal(true);
                        add_to_queue(placeVariables.first);
                    } else if (!_partition[placeVariables.first]
                                    .get_dagonal_tuple_positions()[varPosition.first]) {
                        add_to_queue(placeVariables.first);
                        _partition[placeVariables.first].set_diagonal_tuple_position(
                            varPosition.first, true);
                    }
                    break;
                }
            }
            if (_partition[placeId].is_diagonal()) {
                break;
            }
        }
        if (_partition[placeId].is_diagonal()) {
            break;
        }
    }
}
// Check if the preArc share variables with the postArc and mark diagonal if the
// variable positions are diagonal in the post place
void PartitionBuilder::mark_shared_vars(const PositionVariableMap &preVarPositionMap,
                                        const PositionVariableMap &varPositionMap,
                                        uint32_t postPlaceId, uint32_t prePlaceId) {
    for (const auto &preVar : preVarPositionMap) {
        for (const auto &postVar : varPositionMap) {
            if (preVar.second == postVar.second) {
                if (_partition[postPlaceId].is_diagonal() ||
                    _partition[postPlaceId].get_dagonal_tuple_positions()[postVar.first]) {
                    if (_partition[prePlaceId].get_eq_classes().back().type()->product_size() ==
                        1) {
                        _partition[prePlaceId].set_diagonal(true);
                    } else if (!_partition[prePlaceId]
                                    .get_dagonal_tuple_positions()[preVar.first]) {
                        add_to_queue(prePlaceId);
                        _partition[prePlaceId].set_diagonal_tuple_position(preVar.first, true);
                    }
                }
            }
        }
    }
}
// Check if any of the variables on the preArc was part of a diagonal constraint in the gaurd
void PartitionBuilder::check_var_in_guard(const PositionVariableMap &preVarPositionMap,
                                          const std::set<const Colored::Variable *> &diagonalVars,
                                          uint32_t placeId) {
    for (const auto &preVar : preVarPositionMap) {
        if (diagonalVars.count(preVar.second)) {
            if (_partition[placeId].get_eq_classes().back().type()->product_size() == 1) {
                _partition[placeId].set_diagonal(true);
                break;
            } else if (!_partition[placeId].get_dagonal_tuple_positions()[preVar.first]) {
                add_to_queue(placeId);
                _partition[placeId].set_diagonal_tuple_position(preVar.first, true);
            }
        }
    }
}

// Check if we have marked all positions in the product type of the place as diagonal
// and mark the whole place as diagonal if it is the case
auto PartitionBuilder::check_tuple_diagonal(uint32_t placeId) -> bool {
    bool allPositionsDiagonal = true;
    for (auto diag : _partition[placeId].get_dagonal_tuple_positions()) {
        if (!diag) {
            allPositionsDiagonal = false;
            break;
        }
    }

    if (allPositionsDiagonal) {
        _partition[placeId].set_diagonal(true);
        add_to_queue(placeId);
        return true;
    }
    return false;
}

auto PartitionBuilder::check_diagonal(uint32_t placeId) -> bool {
    if (_partition[placeId].is_diagonal()) {
        add_to_queue(placeId);
        return true;
    }
    return false;
}

void PartitionBuilder::apply_new_intervals(
    const Arc &inArc, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps) {
    // Retrieve the intervals for the current place,
    // based on the intervals from the postPlace, the postArc, preArc and guard
    auto outIntervals = inArc._expr->get_output_intervals(varMaps);
    EquivalenceVec newEqVec;
    for (auto &intervalTuple : outIntervals) {
        intervalTuple.simplify();
        EquivalenceClass newEqClass(++_eq_id_counter,
                                    _partition[inArc._place].get_eq_classes().back().type(),
                                    std::move(intervalTuple));
        newEqVec.push_back_eq_class(newEqClass);
    }
    newEqVec.set_diagonal_tuple_positions(_partition[inArc._place].get_dagonal_tuple_positions());

    // If the prePlace has not been marked as diagonal, then split the current partitions based on
    // the new intervals
    if (split_partition(std::move(newEqVec), inArc._place)) {
        add_to_queue(inArc._place);
    }
    _partition[inArc._place].merge_eq_classes();
}

void PartitionBuilder::handle_transition(const Transition &transition, const uint32_t postPlaceId,
                                         const Arc *postArc) {
    VariableModifierMap varModifierMap;
    PositionVariableMap varPositionMap;
    std::set<const PetriEngine::Colored::Variable *> postArcVars;
    std::set<const PetriEngine::Colored::Variable *> guardVars;
    std::set<const Colored::Variable *> diagonalVars;

    postArc->_expr->get_variables(postArcVars, varPositionMap, varModifierMap, true);

    check_var_on_arc(varModifierMap, diagonalVars, postPlaceId, false);

    if (transition._guard != nullptr) {
        transition._guard->get_variables(guardVars);
    }
    // we have to copy here, the following loop has the *potential* to modify
    // _partition[postPlaceId]
    const std::vector<Colored::EquivalenceClass> placePartition =
        _partition[postPlaceId].get_eq_classes();

    // Partition each of the equivalence classes
    for (const auto &eqClass : placePartition) {
        auto varMaps = prepare_variables(varModifierMap, eqClass, postArc, postPlaceId);

        // If there are variables in the guard, that doesn't come from the postPlace
        // we give them the full interval
        for (auto &varMap : varMaps) {
            for (auto *var : guardVars) {
                if (varMap.count(var) == 0) {
                    varMap[var].add_interval(var->_colorType->get_full_interval());
                }
            }
        }
        if (transition._guard != nullptr) {
            transition._guard->restrict_vars(varMaps, diagonalVars);
        }

        handle_in_arcs(transition, diagonalVars, varPositionMap, varMaps, postPlaceId);
    }
}

void PartitionBuilder::handle_in_arcs(
    const Transition &transition, std::set<const Colored::Variable *> &diagonalVars,
    const PositionVariableMap &varPositionMap,
    const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId) {
    std::unordered_map<uint32_t, PositionVariableMap> placeVariableMap;
    for (const auto &inArc : transition._input_arcs) {
        // Hack to avoid considering dot places
        if (_places[inArc._place]._type == ColorType::dot_instance()) {
            _partition[inArc._place].set_diagonal(true);
        }

        if (_partition[inArc._place].is_diagonal()) {
            continue;
        }
        VariableModifierMap preVarModifierMap;
        PositionVariableMap preVarPositionMap;
        std::set<const PetriEngine::Colored::Variable *> preArcVars;
        inArc._expr->get_variables(preArcVars, preVarPositionMap, preVarModifierMap, true);
        check_var_on_input_arcs(placeVariableMap, preVarPositionMap, diagonalVars, inArc._place);
        placeVariableMap[inArc._place] = preVarPositionMap;
        if (check_diagonal(inArc._place))
            continue;

        mark_shared_vars(preVarPositionMap, varPositionMap, postPlaceId, inArc._place);
        if (check_diagonal(inArc._place))
            continue;
        check_var_on_arc(preVarModifierMap, diagonalVars, inArc._place, true);
        if (check_diagonal(inArc._place))
            continue;

        check_var_in_guard(preVarPositionMap, diagonalVars, inArc._place);
        if (check_diagonal(inArc._place))
            continue;

        _partition[inArc._place].merge_eq_classes();
        if (_partition[inArc._place].get_eq_classes().size() >=
            _partition[inArc._place].get_eq_classes().back().type()->size(
                _partition[inArc._place].get_dagonal_tuple_positions())) {
            _partition[inArc._place].set_diagonal(true);
            continue;
        }

        if (check_tuple_diagonal(inArc._place)) {
            continue;
        }

        apply_new_intervals(inArc, varMaps);
    }
}

void PartitionBuilder::add_to_queue(uint32_t placeId) {
    if (!_inQueue[placeId]) {
        _placeQueue.push_back(placeId);
        _inQueue[placeId] = true;
    }
}

auto PartitionBuilder::split_partition(PetriEngine::Colored::EquivalenceVec equivalenceVec,
                                       uint32_t placeId) -> bool {
    bool split = false;
    if (_partition.count(placeId) == 0) {
        _partition[placeId] = equivalenceVec;
    } else {
        EquivalenceClass intersection(++_eq_id_counter);
        uint32_t ecPos1 = 0, ecPos2 = 0;
        while (find_overlap(equivalenceVec, _partition[placeId], ecPos1, ecPos2, intersection)) {
            const auto &ec1 = equivalenceVec.get_eq_classes()[ecPos1];
            const auto &ec2 = _partition[placeId].get_eq_classes()[ecPos2];
            const auto rightSubtractEc =
                ec1.subtract(++_eq_id_counter, ec2, equivalenceVec.get_dagonal_tuple_positions());
            const auto leftSubtractEc = ec2.subtract(
                ++_eq_id_counter, ec1, _partition[placeId].get_dagonal_tuple_positions());

            equivalenceVec.erase_eq_class(ecPos1);
            _partition[placeId].erase_eq_class(ecPos2);

            if (!intersection.is_empty()) {
                _partition[placeId].push_back_eq_class(intersection);
                intersection.clear();
            }
            if (!leftSubtractEc.is_empty()) {
                _partition[placeId].push_back_eq_class(leftSubtractEc);
                split = true;
            }
            if (!rightSubtractEc.is_empty()) {
                equivalenceVec.push_back_eq_class(rightSubtractEc);
            }
        }
    }
    return split;
}

auto PartitionBuilder::find_overlap(const EquivalenceVec &equivalenceVec1,
                                    const EquivalenceVec &equivalenceVec2, uint32_t &overlap1,
                                    uint32_t &overlap2, EquivalenceClass &intersection) -> bool {
    for (uint32_t i = 0; i < equivalenceVec1.get_eq_classes().size(); i++) {
        for (uint32_t j = 0; j < equivalenceVec2.get_eq_classes().size(); j++) {
            const auto &ec = equivalenceVec1.get_eq_classes()[i];
            const auto &ec2 = equivalenceVec2.get_eq_classes()[j];

            auto intersectingEc = ec.intersect(++_eq_id_counter, ec2);
            if (!intersectingEc.is_empty()) {
                overlap1 = i;
                overlap2 = j;
                intersection = intersectingEc;
                return true;
            }
        }
    }
    return false;
}

auto PartitionBuilder::prepare_variables(const VariableModifierMap &varModifierMap,
                                         const EquivalenceClass &eqClass, const Arc *arc,
                                         uint32_t placeId) -> std::vector<VariableIntervalMap> {
    std::vector<VariableIntervalMap> varMaps;
    VariableIntervalMap varMap;
    varMaps.push_back(varMap);
    std::unordered_map<uint32_t, arc_intervals_t> placeArcIntervals;
    ColorFixpoint postPlaceFixpoint;
    postPlaceFixpoint._constraints = eqClass.intervals();
    arc_intervals_t newArcInterval(&postPlaceFixpoint, varModifierMap);
    uint32_t index = 0;

    arc->_expr->get_arc_intervals(newArcInterval, postPlaceFixpoint, index, 0);
    placeArcIntervals[placeId] = newArcInterval;
    _interval_generator.get_var_intervals(varMaps, placeArcIntervals);

    return varMaps;
}

void PartitionBuilder::handle_leaf_transitions() {
    for (const auto &transition : _transitions) {
        if (transition._output_arcs.empty() && !transition._input_arcs.empty()) {
            handle_transition(transition, transition._input_arcs.back()._place,
                              &transition._input_arcs.back());
        }
    }
}
} // namespace PetriEngine::Colored
