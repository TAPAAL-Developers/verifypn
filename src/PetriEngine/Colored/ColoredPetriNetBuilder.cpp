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

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include <chrono>
#include <tuple>
using std::get;
namespace PetriEngine {
ColoredPetriNetBuilder::ColoredPetriNetBuilder() = default;

ColoredPetriNetBuilder::ColoredPetriNetBuilder(const ColoredPetriNetBuilder &orig)
    : AbstractPetriNetBuilder(orig), _placenames(orig._placenames),
      _transitionnames(orig._transitionnames), _places(orig._places),
      _transitions(orig._transitions) {}

ColoredPetriNetBuilder::~ColoredPetriNetBuilder() {
    // cleaning up colors
    for (auto &e : _colors) {
        if (e.second != Colored::ColorType::dot_instance())
            delete e.second;
    }
    _colors.clear();
}

void ColoredPetriNetBuilder::add_place(const std::string &name, uint32_t tokens, double x,
                                       double y) {
    if (!_isColored) {
        _ptBuilder.add_place(name, tokens, x, y);
    }
}

void ColoredPetriNetBuilder::add_place(const std::string &name, const Colored::ColorType *type,
                                       Colored::Multiset &&tokens, double x, double y) {
    if (_placenames.count(name) == 0) {
        uint32_t next = _placenames.size();
        _places.emplace_back(Colored::place_t{name, type, tokens});
        _placenames[name] = next;

        // set up place color fix points and initialize queue
        if (!tokens.empty()) {
            _placeFixpointQueue.emplace_back(next);
        }

        Colored::IntervalVector placeConstraints;
        Colored::color_fixpoint_t colorFixpoint = {placeConstraints, !tokens.empty()};
        uint32_t colorCounter = 0;

        if (tokens.size() == type->size()) {
            for (const auto &colorPair : tokens) {
                if (colorPair.second > 0) {
                    colorCounter++;
                } else {
                    break;
                }
            }
        }

        if (colorCounter == type->size()) {
            colorFixpoint._constraints.add_interval(type->get_full_interval());
        } else {
            for (const auto &colorPair : tokens) {
                Colored::interval_t tokenConstraints;
                uint32_t index = 0;
                colorPair.first->get_color_constraints(tokenConstraints, index);

                colorFixpoint._constraints.add_interval(tokenConstraints);
            }
        }

        _placeColorFixpoints.push_back(colorFixpoint);
    }
}

void ColoredPetriNetBuilder::add_transition(const std::string &name, double x, double y) {
    if (!_isColored) {
        _ptBuilder.add_transition(name, x, y);
    }
}

void ColoredPetriNetBuilder::add_transition(const std::string &name,
                                            const Colored::GuardExpression_ptr &guard, double x,
                                            double y) {
    if (_transitionnames.count(name) == 0) {
        uint32_t next = _transitionnames.size();
        _transitions.emplace_back(Colored::transition_t{name, guard});
        _transitionnames[name] = next;
    }
}

void ColoredPetriNetBuilder::add_input_arc(const std::string &place, const std::string &transition,
                                           bool inhibitor, uint32_t weight) {
    if (!_isColored) {
        _ptBuilder.add_input_arc(place, transition, inhibitor, weight);
    }
}

void ColoredPetriNetBuilder::add_input_arc(const std::string &place, const std::string &transition,
                                           const Colored::ArcExpression_ptr &expr, bool inhibitor,
                                           uint32_t weight) {
    add_arc(place, transition, expr, true, inhibitor, weight);
}

void ColoredPetriNetBuilder::add_output_arc(const std::string &transition, const std::string &place,
                                            uint32_t weight) {
    if (!_isColored) {
        _ptBuilder.add_output_arc(transition, place, weight);
    }
}

void ColoredPetriNetBuilder::add_output_arc(const std::string &transition, const std::string &place,
                                            const Colored::ArcExpression_ptr &expr) {
    add_arc(place, transition, expr, false, false, 1);
}

void ColoredPetriNetBuilder::add_arc(const std::string &place, const std::string &transition,
                                     const Colored::ArcExpression_ptr &expr, bool input,
                                     bool inhibitor, int weight) {
    if (_transitionnames.count(transition) == 0) {
        std::cout << "Transition '" << transition << "' not found. Adding it." << std::endl;
        add_transition(transition, 0.0, 0.0);
    }
    if (_placenames.count(place) == 0) {
        std::cout << "Place '" << place << "' not found. Adding it." << std::endl;
        add_place(place, 0, 0, 0);
    }
    uint32_t p = _placenames[place];
    uint32_t t = _transitionnames[transition];

    assert(t < _transitions.size());
    assert(p < _places.size());

    input ? _placePostTransitionMap[p].emplace_back(t) : _placePreTransitionMap[p].emplace_back(t);

    Colored::arc_t arc;
    arc._place = p;
    arc._transition = t;
    _places[p]._inhibitor |= inhibitor;
    if (!inhibitor)
        assert(expr != nullptr);
    arc._expr = expr;
    arc._input = input;
    arc._weight = weight;
    if (inhibitor) {
        _inhibitorArcs.push_back(std::move(arc));
    } else {
        input ? _transitions[t]._input_arcs.push_back(std::move(arc))
              : _transitions[t]._output_arcs.push_back(std::move(arc));
    }
}

void ColoredPetriNetBuilder::add_color_type(const std::string &id, const Colored::ColorType *type) {
    _colors[id] = type;
}

void ColoredPetriNetBuilder::sort() {}

//------------------- Symmetric Variables --------------------//

void ColoredPetriNetBuilder::compute_symmetric_variables() {
    if (_isColored) {
        for (uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++) {
            const Colored::transition_t &transition = _transitions[transitionId];
            if (transition._guard) {
                continue;
                // the variables cannot appear on the guard
            }

            for (const auto &inArc : transition._input_arcs) {
                std::set<const Colored::Variable *> inArcVars;
                std::vector<uint32_t> numbers;

                // Application of symmetric variables for partitioned places is currently unhandled
                if (_partitionComputed && !_partition[inArc._place].is_diagonal()) {
                    continue;
                }

                // the expressions is eligible if it is an addexpression that contains only
                // numberOfExpressions with the same number
                bool isEligible = inArc._expr->is_eligible_for_symmetry(numbers);

                if (isEligible && numbers.size() > 1) {
                    inArc._expr->get_variables(inArcVars);
                    // It cannot be symmetric with anything if there is only one variable
                    if (inArcVars.size() < 2) {
                        continue;
                    }
                    // The variables may appear only on one input arc and one output arc
                    check_symmetric_vars_in_arcs(transition, inArc, inArcVars, isEligible);

                    // All the variables have to appear on exactly one output arc and nowhere else
                    check_symmetric_vars_out_arcs(transition, inArcVars, isEligible);
                } else {
                    isEligible = false;
                }
                if (isEligible) {
                    _symmetric_var_map[transitionId].emplace_back(inArcVars);
                }
            }
        }
    }
}

void ColoredPetriNetBuilder::check_symmetric_vars_in_arcs(
    const Colored::transition_t &transition, const Colored::arc_t &inArc,
    const std::set<const Colored::Variable *> &inArcVars, bool &isEligible) const {
    for (auto &otherInArc : transition._input_arcs) {
        if (inArc._place == otherInArc._place) {
            continue;
        }
        std::set<const Colored::Variable *> otherArcVars;
        otherInArc._expr->get_variables(otherArcVars);
        for (auto *var : inArcVars) {
            if (otherArcVars.find(var) != otherArcVars.end()) {
                isEligible = false;
                break;
            }
        }
    }
}

void ColoredPetriNetBuilder::check_symmetric_vars_out_arcs(
    const Colored::transition_t &transition, const std::set<const Colored::Variable *> &inArcVars,
    bool &isEligible) const {
    uint32_t numArcs = 0;
    bool foundSomeVars = false;
    for (auto &outputArc : transition._output_arcs) {
        bool foundArc = true;
        std::set<const Colored::Variable *> otherArcVars;
        outputArc._expr->get_variables(otherArcVars);
        for (auto *var : inArcVars) {
            if (otherArcVars.find(var) == otherArcVars.end()) {
                foundArc = false;
            } else {
                foundSomeVars = true;
            }
        }
        if (foundArc) {
            // Application of symmetric variables for partitioned places is currently unhandled
            if (_partitionComputed && !_partition.find(outputArc._place)->second.is_diagonal()) {
                isEligible = false;
                break;
            }
            numArcs++;
            // All vars were present
            foundSomeVars = false;
        }
        // If some vars are present the vars are not eligible
        if (foundSomeVars) {
            isEligible = false;
            break;
        }
    }

    if (numArcs != 1) {
        isEligible = false;
    }
}

void ColoredPetriNetBuilder::print_symmetric_variables() const {
    for (uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++) {
        const auto &transition = _transitions[transitionId];
        if (_symmetric_var_map.find(transitionId) == _symmetric_var_map.end()) {
            std::cout << "Transition " << transition._name << " has no symmetric variables"
                      << std::endl;
        } else {
            std::cout << "Transition " << transition._name
                      << " has symmetric variables: " << std::endl;
            for (const auto &set : _symmetric_var_map.find(transitionId)->second) {
                std::string toPrint = "SET: ";
                for (auto *variable : set) {
                    toPrint += variable->_name + ", ";
                }
                std::cout << toPrint << std::endl;
            }
        }
    }
}

//----------------------- Partitioning -----------------------//

void ColoredPetriNetBuilder::compute_partition(int32_t timeout) {
    if (_isColored) {
        auto partitionStart = std::chrono::high_resolution_clock::now();
        Colored::PartitionBuilder pBuilder =
            _fixpointDone
                ? Colored::PartitionBuilder(_transitions, _places, _placePostTransitionMap,
                                            _placePreTransitionMap, &_placeColorFixpoints)
                : Colored::PartitionBuilder(_transitions, _places, _placePostTransitionMap,
                                            _placePreTransitionMap);

        if (pBuilder.partition_net(timeout)) {
            // pBuilder.printPartion();
            _partition = pBuilder.get_partition();
            pBuilder.assign_color_map(_partition);
            _partitionComputed = true;
        }
        auto partitionEnd = std::chrono::high_resolution_clock::now();
        _partitionTimer =
            (std::chrono::duration_cast<std::chrono::microseconds>(partitionEnd - partitionStart)
                 .count()) *
            0.000001;
    }
}

//----------------------- Color fixpoint -----------------------//

void ColoredPetriNetBuilder::print_place_table() const {
    for (const auto &place : _places) {
        const auto &placeID = _placenames.find(place._name)->second;
        const auto &placeColorFixpoint = _placeColorFixpoints[placeID];
        std::cout << "Place: " << place._name << " in queue: " << placeColorFixpoint._in_queue
                  << " with colortype " << place._type->get_name() << std::endl;

        for (const auto &fixpointPair : placeColorFixpoint._constraints) {
            std::cout << "[";
            for (const auto &range : fixpointPair._ranges) {
                std::cout << range._lower << "-" << range._upper << ", ";
            }
            std::cout << "]" << std::endl;
        }
        std::cout << std::endl;
    }
}

void ColoredPetriNetBuilder::compute_place_color_fixpoint(uint32_t maxIntervals,
                                                          uint32_t maxIntervalsReduced,
                                                          int32_t timeout) {
    if (_isColored) {
        // Start timers for timing color fixpoint creation and max interval reduction steps
        auto start = std::chrono::high_resolution_clock::now();
        auto end = std::chrono::high_resolution_clock::now();
        auto reduceTimer = std::chrono::high_resolution_clock::now();
        while (!_placeFixpointQueue.empty()) {
            // Reduce max interval once timeout passes
            if (maxIntervals > maxIntervalsReduced && timeout > 0 &&
                std::chrono::duration_cast<std::chrono::seconds>(end - reduceTimer).count() >=
                    timeout) {
                maxIntervals = maxIntervalsReduced;
            }

            uint32_t currentPlaceId = _placeFixpointQueue.back();
            _placeFixpointQueue.pop_back();
            _placeColorFixpoints[currentPlaceId]._in_queue = false;
            std::vector<uint32_t> connectedTransitions = _placePostTransitionMap[currentPlaceId];

            for (uint32_t transitionId : connectedTransitions) {
                Colored::transition_t &transition = _transitions[transitionId];
                // Skip transitions that cannot add anything new,
                // such as transitions with only constants on their arcs that have been processed
                // once
                if (transition._considered)
                    continue;
                bool transitionActivated = true;
                transition._variable_maps.clear();

                if (!_arcIntervals.count(transitionId)) {
                    _arcIntervals[transitionId] = setup_transition_vars(transition);
                }
                process_input_arcs(transition, currentPlaceId, transitionId, transitionActivated,
                                   maxIntervals);

                // If there were colors which activated the transitions, compute the intervals
                // produced
                if (transitionActivated) {
                    process_output_arcs(transition);
                }
            }
            end = std::chrono::high_resolution_clock::now();
        }

        _fixpointDone = true;
        _fixPointCreationTime =
            (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) * 0.000001;

        // printPlaceTable();
        _placeColorFixpoints.clear();
    }
}

// Create Arc interval structures for the transition

auto ColoredPetriNetBuilder::setup_transition_vars(const Colored::transition_t &transition) const
    -> std::unordered_map<uint32_t, Colored::arc_intervals_t> {
    std::unordered_map<uint32_t, Colored::arc_intervals_t> res;
    for (auto &arc : transition._input_arcs) {
        std::set<const Colored::Variable *> variables;
        Colored::PositionVariableMap varPositions;
        Colored::VariableModifierMap varModifiersMap;
        arc._expr->get_variables(variables, varPositions, varModifiersMap, false);

        Colored::arc_intervals_t newArcInterval(&_placeColorFixpoints[arc._place], varModifiersMap);
        res[arc._place] = newArcInterval;
    }
    return res;
}

void ColoredPetriNetBuilder::create_partion_varmaps() {
    for (uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++) {
        Colored::transition_t &transition = _transitions[transitionId];
        std::set<const Colored::Variable *> variables;
        _arcIntervals[transitionId] = setup_transition_vars(transition);

        for (const auto &inArc : transition._input_arcs) {
            Colored::arc_intervals_t &arcInterval = _arcIntervals[transitionId][inArc._place];
            uint32_t index = 0;
            arcInterval._intervalTupleVec.clear();

            Colored::IntervalVector intervalTuple;
            intervalTuple.add_interval(_places[inArc._place]._type->get_full_interval());
            const PetriEngine::Colored::color_fixpoint_t &cfp{intervalTuple};

            inArc._expr->get_arc_intervals(arcInterval, cfp, index, 0);

            _partition[inArc._place].apply_partition(arcInterval);
        }

        _intervalGenerator.get_var_intervals(transition._variable_maps,
                                             _arcIntervals[transitionId]);
        for (const auto &outArc : transition._output_arcs) {
            outArc._expr->get_variables(variables);
        }
        if (transition._guard != nullptr) {
            transition._guard->get_variables(variables);
        }
        for (auto *var : variables) {
            for (auto &varmap : transition._variable_maps) {
                if (varmap.count(var) == 0) {
                    Colored::IntervalVector intervalTuple;
                    intervalTuple.add_interval(var->_colorType->get_full_interval());
                    varmap[var] = intervalTuple;
                }
            }
        }
    }
}

// Retrieve color intervals for the input arcs based on their places

void ColoredPetriNetBuilder::get_arc_intervals(const Colored::transition_t &transition,
                                               bool &transitionActivated, uint32_t max_intervals,
                                               uint32_t transitionId) {
    for (auto &arc : transition._input_arcs) {
        PetriEngine::Colored::color_fixpoint_t &curCFP = _placeColorFixpoints[arc._place];
        curCFP._constraints.restrict(max_intervals);
        _maxIntervals = std::max(_maxIntervals, (uint32_t)curCFP._constraints.size());

        Colored::arc_intervals_t &arcInterval = _arcIntervals[transitionId][arc._place];
        uint32_t index = 0;
        arcInterval._intervalTupleVec.clear();

        if (!arc._expr->get_arc_intervals(arcInterval, curCFP, index, 0)) {
            transitionActivated = false;
            return;
        }

        if (_partitionComputed) {
            _partition[arc._place].apply_partition(arcInterval);
        }
    }
}

void ColoredPetriNetBuilder::add_transition_vars(Colored::transition_t &transition) const {
    std::set<const Colored::Variable *> variables;
    transition._guard->get_variables(variables);
    for (auto *var : variables) {
        for (auto &varmap : transition._variable_maps) {
            if (varmap.count(var) == 0) {
                Colored::IntervalVector intervalTuple;
                intervalTuple.add_interval(var->_colorType->get_full_interval());
                varmap[var] = intervalTuple;
            }
        }
    }
}

void ColoredPetriNetBuilder::remove_invalid_varmaps(Colored::transition_t &transition) const {
    std::vector<Colored::VariableIntervalMap> newVarmaps;
    for (auto &varMap : transition._variable_maps) {
        bool validVarMap = true;
        for (auto &varPair : varMap) {
            if (!varPair.second.has_valid_intervals()) {
                validVarMap = false;
                break;
            } else {
                varPair.second.simplify();
            }
        }
        if (validVarMap) {
            newVarmaps.push_back(std::move(varMap));
        }
    }
    transition._variable_maps = std::move(newVarmaps);
}

// Retreive interval colors from the input arcs restricted by the transition guard

void ColoredPetriNetBuilder::process_input_arcs(Colored::transition_t &transition,
                                                uint32_t currentPlaceId, uint32_t transitionId,
                                                bool &transitionActivated, uint32_t max_intervals) {
    get_arc_intervals(transition, transitionActivated, max_intervals, transitionId);

    if (!transitionActivated) {
        return;
    }
    if (_intervalGenerator.get_var_intervals(transition._variable_maps,
                                             _arcIntervals[transitionId])) {
        if (transition._guard != nullptr) {
            add_transition_vars(transition);
            transition._guard->restrict_vars(transition._variable_maps);
            remove_invalid_varmaps(transition);

            if (transition._variable_maps.empty()) {
                // Guard restrictions removed all valid intervals
                transitionActivated = false;
                return;
            }
        }
    } else {
        // Retrieving variable intervals failed
        transitionActivated = false;
    }
}

void ColoredPetriNetBuilder::process_output_arcs(Colored::transition_t &transition) {
    bool transitionHasVarOutArcs = false;
    for (const auto &arc : transition._output_arcs) {
        Colored::color_fixpoint_t &placeFixpoint = _placeColorFixpoints[arc._place];
        // used to check if colors are added to the place. The total distance between upper and
        // lower bounds should grow when more colors are added and as we cannot remove colors this
        // can be checked by summing the differences
        uint32_t colorsBefore = placeFixpoint._constraints.get_contained_colors();

        std::set<const Colored::Variable *> variables;
        arc._expr->get_variables(variables);

        if (!variables.empty()) {
            transitionHasVarOutArcs = true;
        }

        // If there is a varaible which was not found on an input arc or in the guard, we give it
        // the full interval
        for (auto *var : variables) {
            for (auto &varmap : transition._variable_maps) {
                if (varmap.count(var) == 0) {
                    Colored::IntervalVector intervalTuple;
                    intervalTuple.add_interval(var->_colorType->get_full_interval());
                    varmap[var] = intervalTuple;
                }
            }
        }

        // Apply partitioning to unbound outgoing variables such that
        // bindings are only created for colors used in the rest of the net
        if (_partitionComputed && !_partition[arc._place].is_diagonal()) {
            for (auto *outVar : variables) {
                for (auto &varMap : transition._variable_maps) {
                    if (varMap.count(outVar) == 0) {
                        Colored::IntervalVector varIntervalTuple;
                        for (const auto &EqClass : _partition[arc._place].get_eq_classes()) {
                            varIntervalTuple.add_interval(
                                EqClass.intervals().back().get_single_color_interval());
                        }
                        varMap[outVar] = varIntervalTuple;
                    }
                }
            }
        }

        auto intervals = arc._expr->get_output_intervals(transition._variable_maps);

        for (auto &intervalTuple : intervals) {
            intervalTuple.simplify();
            for (auto &interval : intervalTuple) {
                placeFixpoint._constraints.add_interval(interval);
            }
        }
        placeFixpoint._constraints.simplify();

        // Check if the place should be added to the queue
        if (!placeFixpoint._in_queue) {
            uint32_t colorsAfter = placeFixpoint._constraints.get_contained_colors();
            if (colorsAfter > colorsBefore) {
                _placeFixpointQueue.push_back(arc._place);
                placeFixpoint._in_queue = true;
            }
        }
    }
    // If there are no variables among the out arcs of a transition
    // and it has been activated, there is no reason to cosider it again
    if (!transitionHasVarOutArcs) {
        transition._considered = true;
    }
}

// Find places for which the marking cannot change as all input arcs are matched
// by an output arc with an equivalent arc expression and vice versa

void ColoredPetriNetBuilder::find_stable_places() {
    for (uint32_t placeId = 0; placeId < _places.size(); placeId++) {
        if (_placePostTransitionMap.count(placeId) != 0 &&
            !_placePostTransitionMap[placeId].empty() &&
            _placePostTransitionMap[placeId].size() == _placePreTransitionMap[placeId].size()) {

            for (auto transitionId : _placePostTransitionMap[placeId]) {
                bool matched = false;
                for (auto transitionId2 : _placePreTransitionMap[placeId]) {
                    if (transitionId == transitionId2) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    _places[placeId]._stable = false;
                    break;
                }
                const Colored::arc_t *inArc;
                for (const auto &arc : _transitions[transitionId]._input_arcs) {
                    if (arc._place == placeId) {
                        inArc = &arc;
                        break;
                    }
                }
                bool mirroredArcs = false;
                for (auto &arc : _transitions[transitionId]._output_arcs) {
                    if (arc._place == placeId) {
                        if (arc._expr->to_string() == inArc->_expr->to_string()) {
                            mirroredArcs = true;
                        }
                        break;
                    }
                }
                if (!mirroredArcs) {
                    _places[placeId]._stable = false;
                    break;
                }
            }
        } else {
            _places[placeId]._stable = false;
        }
    }
}

//----------------------- Unfolding -----------------------//

auto ColoredPetriNetBuilder::unfold() -> PetriNetBuilder & {
    if (_stripped)
        assert(false);
    if (_isColored && !_unfolded) {
        auto start = std::chrono::high_resolution_clock::now();

        if (_fixpointDone) {
            find_stable_places();
        }

        if (!_fixpointDone && _partitionComputed) {
            create_partion_varmaps();
        }

        for (uint32_t transitionId = 0; transitionId < _transitions.size(); transitionId++) {
            unfold_transition(transitionId);
        }

        const auto &unfoldedPlaceMap = _ptBuilder.get_place_names();
        for (auto &place : _places) {
            handle_orphan_place(place, unfoldedPlaceMap);
        }

        _unfolded = true;
        auto end = std::chrono::high_resolution_clock::now();
        _time =
            (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) * 0.000001;
    }
    return _ptBuilder;
}

// Due to the way we unfold places, we only unfold palces connected to an arc (which makes sense)
// However, in queries asking about orphan places it cannot find these, as they have not been
// unfolded so we make a placeholder place which just has tokens equal to the number of colored
// tokens Ideally, orphan places should just be translated to a constant in the query

void ColoredPetriNetBuilder::handle_orphan_place(
    const Colored::place_t &place,
    const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap) {
    if (_ptplacenames.count(place._name) <= 0 && place._marking.size() > 0) {
        const std::string &name = place._name + "_orphan";
        _ptBuilder.add_place(name, place._marking.size(), 0.0, 0.0);
        _ptplacenames[place._name][0] = name;
    } else {
        uint32_t usedTokens = 0;

        for (const auto &unfoldedPlace : _ptplacenames[place._name]) {
            auto unfoldedMarking = _ptBuilder.init_marking();
            usedTokens += unfoldedMarking[unfoldedPlaceMap.find(unfoldedPlace.second)->second];
        }

        if (place._marking.size() > usedTokens) {
            const std::string &name = place._name + "_orphan";
            _ptBuilder.add_place(name, place._marking.size() - usedTokens, 0.0, 0.0);
            _ptplacenames[place._name][UINT32_MAX] = name;
        }
    }
}

void ColoredPetriNetBuilder::unfold_place(const Colored::place_t *place,
                                          const PetriEngine::Colored::Color *color,
                                          uint32_t placeId, uint32_t id) {
    size_t tokenSize = 0;

    if (!_partitionComputed || _partition[placeId].is_diagonal()) {
        tokenSize = place->_marking[color];
    } else {
        const std::vector<const Colored::Color *> &tupleColors = color->get_tuple_colors();
        const size_t &tupleSize = _partition[placeId].get_dagonal_tuple_positions().size();
        const uint32_t &classId =
            _partition[placeId].get_color_eq_class_map().find(color)->second->id();
        const auto &diagonalTuplePos = _partition[placeId].get_dagonal_tuple_positions();

        for (const auto &colorEqClassPair : _partition[placeId].get_color_eq_class_map()) {
            if (colorEqClassPair.second->id() == classId) {
                const std::vector<const Colored::Color *> &testColors =
                    colorEqClassPair.first->get_tuple_colors();
                bool match = true;
                for (uint32_t i = 0; i < tupleSize; i++) {
                    if (diagonalTuplePos[i] &&
                        tupleColors[i]->get_id() != testColors[i]->get_id()) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    tokenSize += place->_marking[colorEqClassPair.first];
                }
            }
        }
    }
    const std::string &name = place->_name + "_" + std::to_string(color->get_id());

    _ptBuilder.add_place(name, tokenSize, 0.0, 0.0);
    _ptplacenames[place->_name][id] = name;
}

void ColoredPetriNetBuilder::unfold_transition(uint32_t transitionId) {
    const Colored::transition_t &transition = _transitions[transitionId];

    if (_fixpointDone || _partitionComputed) {
        FixpointBindingGenerator gen(transition, _colors, _symmetric_var_map[transitionId]);
        size_t i = 0;
        bool hasBindings = false;
        for (const auto &b : gen) {
            const std::string &name = transition._name + "_" + std::to_string(i++);

            hasBindings = true;
            _ptBuilder.add_transition(name, 0.0, 0.0);

            for (auto &arc : transition._input_arcs) {
                unfold_arc(arc, b, name);
            }
            for (auto &arc : transition._output_arcs) {
                unfold_arc(arc, b, name);
            }

            _pttransitionnames[transition._name].push_back(name);
            unfold_inhibitor_arc(transition._name, name);
        }
        if (!hasBindings) {
            _pttransitionnames[transition._name] = std::vector<std::string>();
        }
    } else {
        NaiveBindingGenerator gen(transition, _colors);
        size_t i = 0;
        for (const auto &b : gen) {
            const std::string &name = transition._name + "_" + std::to_string(i++);
            _ptBuilder.add_transition(name, 0.0, 0.0);

            for (const auto &arc : transition._input_arcs) {
                unfold_arc(arc, b, name);
            }
            for (const auto &arc : transition._output_arcs) {
                unfold_arc(arc, b, name);
            }
            _pttransitionnames[transition._name].push_back(name);
            unfold_inhibitor_arc(transition._name, name);
        }
    }
}

void ColoredPetriNetBuilder::unfold_inhibitor_arc(const std::string &oldname,
                                                  const std::string &newname) {
    for (auto &_inhibitorArc : _inhibitorArcs) {
        if (_transitions[_inhibitorArc._transition]._name.compare(oldname) == 0) {
            const Colored::arc_t &inhibArc = _inhibitorArc;
            const std::string &placeName = _sumPlacesNames[inhibArc._place];

            if (placeName.empty()) {
                const PetriEngine::Colored::place_t &place = _places[inhibArc._place];
                const std::string &sumPlaceName = place._name + "Sum";
                _ptBuilder.add_place(sumPlaceName, place._marking.size(), 0.0, 0.0);
                if (_ptplacenames.count(place._name) <= 0) {
                    _ptplacenames[place._name][0] = sumPlaceName;
                }
                _sumPlacesNames[inhibArc._place] = sumPlaceName;
            }
            _ptBuilder.add_input_arc(placeName, newname, true, inhibArc._weight);
        }
    }
}

void ColoredPetriNetBuilder::unfold_arc(const Colored::arc_t &arc, const Colored::BindingMap &binding,
                                        const std::string &tName) {
    const PetriEngine::Colored::place_t &place = _places[arc._place];
    // If the place is stable, the arc does not need to be unfolded.
    // This exploits the fact that since the transition is being unfolded with this binding
    // we know that this place contains the tokens to activate the transition for this binding
    // because color fixpoint allowed the binding
    if (_fixpointDone && place._stable) {
        return;
    }

    const Colored::expression_context_t &context{binding, _colors, _partition[arc._place]};
    const auto &ms = arc._expr->eval(context);
    uint32_t shadowWeight = 0;

    const Colored::Color *newColor;
    std::vector<uint32_t> tupleIds;
    for (const auto &color : ms) {
        if (color.second == 0) {
            continue;
        }

        if (!_partitionComputed || _partition[arc._place].is_diagonal()) {
            newColor = color.first;
        } else {
            tupleIds.clear();
            color.first->get_tuple_id(tupleIds);

            _partition[arc._place].apply_partition(tupleIds);
            newColor = place._type->get_color(tupleIds);
        }

        shadowWeight += color.second;
        uint32_t id;
        if (!_partitionComputed || _partition[arc._place].is_diagonal()) {
            id = newColor->get_id();
        } else {
            id = _partition[arc._place].get_color_eq_class_map().find(newColor)->second->id() +
                 newColor->get_id();
        }
        const std::string &pName = _ptplacenames[place._name][id];
        if (pName.empty()) {
            unfold_place(&place, newColor, arc._place, id);
        }

        if (arc._input) {
            _ptBuilder.add_input_arc(pName, tName, false, color.second);
        } else {
            _ptBuilder.add_output_arc(tName, pName, color.second);
        }
        ++_nptarcs;
    }

    if (place._inhibitor) {
        const std::string &sumPlaceName = _sumPlacesNames[arc._place];
        if (sumPlaceName.empty()) {
            const std::string &newSumPlaceName = place._name + "Sum";
            _ptBuilder.add_place(newSumPlaceName, place._marking.size(), 0.0, 0.0);
            _sumPlacesNames[arc._place] = newSumPlaceName;
        }

        if (shadowWeight > 0) {
            if (!arc._input) {
                _ptBuilder.add_output_arc(tName, sumPlaceName, shadowWeight);
            } else {
                _ptBuilder.add_input_arc(sumPlaceName, tName, false, shadowWeight);
            }
            ++_nptarcs;
        }
    }
}

//----------------------- Strip Colors -----------------------//

auto ColoredPetriNetBuilder::strip_colors() -> PetriNetBuilder & {
    if (_unfolded)
        assert(false);
    if (_isColored && !_stripped) {
        for (auto &place : _places) {
            _ptBuilder.add_place(place._name, place._marking.size(), 0.0, 0.0);
        }

        for (auto &transition : _transitions) {
            _ptBuilder.add_transition(transition._name, 0.0, 0.0);
            for (const auto &arc : transition._input_arcs) {
                try {
                    _ptBuilder.add_input_arc(_places[arc._place]._name,
                                             _transitions[arc._transition]._name, false,
                                             arc._expr->weight());
                } catch (Colored::WeightException &e) {
                    throw base_error_t("Exception on input arc: ", arc_to_string(arc),
                                     "\n", "In expression: ", arc._expr->to_string(), "\n\t\t",
                                     e.what());
                }
            }
            for (const auto &arc : transition._output_arcs) {
                try {
                    _ptBuilder.add_output_arc(_transitions[arc._transition]._name,
                                              _places[arc._place]._name, arc._expr->weight());
                } catch (Colored::WeightException &e) {
                    throw base_error_t("Exception on output arc: ", arc_to_string(arc),
                                     "\n", "In expression: ", arc._expr->to_string(), "\n", "\t",
                                     e.what());
                }
            }
            for (const auto &arc : _inhibitorArcs) {
                _ptBuilder.add_input_arc(_places[arc._place]._name,
                                         _transitions[arc._transition]._name, true, arc._weight);
            }
        }

        _stripped = true;
        _isColored = false;
    }

    return _ptBuilder;
}

auto ColoredPetriNetBuilder::arc_to_string(const Colored::arc_t &arc) const -> std::string {
    return !arc._input
               ? "(" + _transitions[arc._transition]._name + ", " + _places[arc._place]._name + ")"
               : "(" + _places[arc._place]._name + ", " + _transitions[arc._transition]._name + ")";
}
} // namespace PetriEngine
