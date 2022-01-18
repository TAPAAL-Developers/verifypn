/* Copyright (C) 2020  Peter G. Jensen <root@petergjoel.dk>,
 *                     Alexander Bilgram <alexander@bilgram.dk>,
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



#include "PetriEngine/Colored/ColorOverapprox.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"

namespace PetriEngine {

    ColorOverapprox::ColorOverapprox(const ColoredPetriNetBuilder& builder)
    : _builder(builder) {
    }

    void ColorOverapprox::compute(uint32_t maxIntervals, uint32_t maxIntervalsReduced, int32_t timeout) {
        if (_builder.isColored()) {
            _considered.clear();
            _var_map.clear();
            _var_map.resize(_builder.transitions().size());
            _considered.resize(_builder.transitions().size());
            std::cerr << "START CFP " << std::endl;
            for (size_t pid = 0; pid < _builder.places().size(); ++pid) {
                init_place(pid);
                if (!_builder.places()[pid].marking.empty())
                    _placeFixpointQueue.push_back(pid);
            }
            //Start timers for timing color fixpoint creation and max interval reduction steps
            auto start = std::chrono::high_resolution_clock::now();
            auto end = std::chrono::high_resolution_clock::now();
            auto reduceTimer = std::chrono::high_resolution_clock::now();
            while (!_placeFixpointQueue.empty()) {
                //Reduce max interval once timeout passes
                if (maxIntervals > maxIntervalsReduced && timeout > 0 && std::chrono::duration_cast<std::chrono::seconds>(end - reduceTimer).count() >= timeout) {
                    maxIntervals = maxIntervalsReduced;
                }

                uint32_t currentPlaceId = _placeFixpointQueue.back();
                _placeFixpointQueue.pop_back();
                _placeColorFixpoints[currentPlaceId].inQueue = false;
                const std::vector<uint32_t>& connectedTransitions = _builder.place_postset(currentPlaceId);
                std::cerr << "Processing place : " << _builder.places()[currentPlaceId].name << std::endl;
                print();
                for (uint32_t transitionId : connectedTransitions) {
                    const Colored::Transition& transition = _builder.transitions()[transitionId];
                    std::cerr << "Processing " << transition.name << std::endl;
                    // Skip transitions that cannot add anything new,
                    // such as transitions with only constants on their arcs that have been processed once
                    if (_considered[transitionId]) continue;
                    _var_map[transitionId].clear();

                    if (!_arcIntervals.count(transitionId)) {
                        _arcIntervals[transitionId] = default_transition_intervals(transition);
                    }
                    bool transitionActivated = process_input_arcs(transitionId, currentPlaceId, maxIntervals);

                    //If there were colors which activated the transitions, compute the intervals produced
                    if (transitionActivated) {
                        process_output_arcs(transitionId);
                    }
                }
                end = std::chrono::high_resolution_clock::now();
            }

            _fixpointDone = true;
            _fixPointCreationTime = (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count())*0.000001;
            std::cerr << "END CFP " << std::endl;
            _placeColorFixpoints.clear();
        }
    }

    void ColorOverapprox::print() const {
        for (size_t pid = 0; pid < _builder.places().size(); ++pid) {
            const auto &placeColorFixpoint = _placeColorFixpoints[pid];
            const auto& place = _builder.places()[pid];
            std::cout << "Place: " << place.name << " in queue: " << placeColorFixpoint.inQueue << " with colortype " << place.type->getName() << std::endl;
            for (const auto &fixpointPair : placeColorFixpoint.constraints) {
                std::cout << "[";
                for (const auto &range : fixpointPair._ranges) {
                    std::cout << range._lower << "-" << range._upper << ", ";
                }
                std::cout << "]" << std::endl;
            }
            std::cout << std::endl;
        }
    }

    //Retreive interval colors from the input arcs restricted by the transition guard

    bool ColorOverapprox::process_input_arcs(size_t transition_id, uint32_t currentPlaceId, uint32_t max_intervals) {
        const auto& transition = _builder.transitions()[transition_id];

        if (!make_arc_intervals(transition_id, max_intervals)) {
            return false;
        }
        auto& varmap = _var_map[transition_id];
        if (_intervalGenerator.getVarIntervals(varmap, _arcIntervals[transition_id])) {
            if (transition.guard != nullptr) {
                add_transition_vars(transition_id);
                transition.guard->restrictVars(varmap);
                removeInvalidVarmaps(transition_id);

                if (varmap.empty()) {
                    //Guard restrictions removed all valid intervals
                    return false;
                }
            }
        } else {
            //Retrieving variable intervals failed
            return false;
        }
        return true;
    }

    void ColorOverapprox::process_output_arcs(size_t transition_id) {
        bool transitionHasVarOutArcs = false;
        const auto& transition = _builder.transitions()[transition_id];
        for (const auto& arc : transition.output_arcs) {
            Colored::ColorFixpoint& placeFixpoint = _placeColorFixpoints[arc.place];
            //used to check if colors are added to the place. The total distance between upper and
            //lower bounds should grow when more colors are added and as we cannot remove colors this
            //can be checked by summing the differences
            uint32_t colorsBefore = placeFixpoint.constraints.getContainedColors();

            std::set<const Colored::Variable *> variables;
            arc.expr->getVariables(variables);

            if (!variables.empty()) {
                transitionHasVarOutArcs = true;
            }

            //Apply partitioning to unbound outgoing variables such that
            // bindings are only created for colors used in the rest of the net

            if (_builder.is_partitioned() && !_builder.partitions()[arc.place].isDiagonal()) {
                auto& partition = _builder.partitions()[arc.place];
                for (auto* outVar : variables) {
                    for (auto& varMap : _var_map[transition_id]) {
                        if (varMap.count(outVar) == 0) {
                            Colored::interval_vector_t varIntervalTuple;
                            for (const auto& EqClass : partition) {
                                varIntervalTuple.addInterval(EqClass.intervals().back().getSingleColorInterval());
                            }
                            varMap[outVar] = std::move(varIntervalTuple);
                        }
                    }
                }
            } else {
                // Else if partitioning was not computed or diagonal
                // and there is a varaible which was not found on an input arc or in the guard,
                // we give it the full interval
                for (auto* var : variables) {
                    for (auto& varmap : _var_map[transition_id]) {
                        if (varmap.count(var) == 0) {
                            Colored::interval_vector_t intervalTuple;
                            intervalTuple.addInterval(var->colorType->getFullInterval());
                            varmap[var] = std::move(intervalTuple);
                        }
                    }
                }
            }

            auto intervals = arc.expr->getOutputIntervals(_var_map[transition_id]);


            for (auto& intervalTuple : intervals) {
                intervalTuple.simplify();
                for (auto& interval : intervalTuple) {
                    placeFixpoint.constraints.addInterval(std::move(interval));
                }
            }
            placeFixpoint.constraints.simplify();

            //Check if the place should be added to the queue
            if (!placeFixpoint.inQueue) {
                uint32_t colorsAfter = placeFixpoint.constraints.getContainedColors();
                if (colorsAfter > colorsBefore) {
                    _placeFixpointQueue.push_back(arc.place);
                    placeFixpoint.inQueue = true;
                }
            }
        }
        //If there are no variables among the out arcs of a transition
        // and it has been activated, there is no reason to cosider it again
        if (!transitionHasVarOutArcs) {
            _considered[transition_id] = true;
        }
    }

    void ColorOverapprox::add_transition_vars(size_t transition_id) {
        std::set<const Colored::Variable *> variables;
        _builder.transitions()[transition_id].guard->getVariables(variables);
        for (auto* var : variables) {
            for (auto& varmap : _var_map[transition_id]) {
                if (varmap.count(var) == 0) {
                    Colored::interval_vector_t intervalTuple;
                    intervalTuple.addInterval(var->colorType->getFullInterval());
                    varmap[var] = std::move(intervalTuple);
                }
            }
        }
    }

    void ColorOverapprox::removeInvalidVarmaps(size_t tid) {
        std::vector<Colored::VariableIntervalMap> newVarmaps;
        for (auto& varMap : _var_map[tid]) {
            bool validVarMap = true;
            for (auto& varPair : varMap) {
                if (!varPair.second.hasValidIntervals()) {
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
        _var_map[tid] = std::move(newVarmaps);
    }

    std::unordered_map<uint32_t, Colored::ArcIntervals> ColorOverapprox::default_transition_intervals(const Colored::Transition &transition) const {
        std::unordered_map<uint32_t, Colored::ArcIntervals> res;
        for (auto& arc : transition.input_arcs) {
            std::set<const Colored::Variable *> variables;
            Colored::PositionVariableMap varPositions;
            Colored::VariableModifierMap varModifiersMap;
            arc.expr->getVariables(variables, varPositions, varModifiersMap, false);

            Colored::ArcIntervals newArcInterval(&_placeColorFixpoints[arc.place], varModifiersMap);
            res[arc.place] = newArcInterval;
        }
        return res;
    }


    Colored::ColorFixpoint ColorOverapprox::default_place_fixpoint(uint32_t pid) const {
        Colored::interval_vector_t placeConstraints;
        const auto& place = _builder.places()[pid];
        Colored::ColorFixpoint colorFixpoint = {placeConstraints, !place.marking.empty()};
        uint32_t colorCounter = 0;
        if (place.marking.size() == place.type->size()) {
            for (const auto& colorPair : place.marking) {
                if (colorPair.second > 0) {
                    colorCounter++;
                } else {
                    break;
                }
            }
        }

        if (colorCounter == place.type->size()) {
            colorFixpoint.constraints.addInterval(place.type->getFullInterval());
        } else {
            for (const auto& colorPair : place.marking) {
                Colored::interval_t tokenConstraints;
                uint32_t index = 0;
                colorPair.first->getColorConstraints(tokenConstraints, index);

                colorFixpoint.constraints.addInterval(tokenConstraints);
            }
        }
        return colorFixpoint;
    }

    void ColorOverapprox::init_place(size_t pid) {
        _placeColorFixpoints.emplace_back(default_place_fixpoint(pid));
    }

    //Retrieve color intervals for the input arcs based on their places

    bool ColorOverapprox::make_arc_intervals(uint32_t transitionId, uint32_t max_intervals) {
        const auto& transition = _builder.transitions()[transitionId];
        for (auto& arc : transition.input_arcs) {
            auto& curCFP = _placeColorFixpoints[arc.place];
            curCFP.constraints.restrict(max_intervals);
            _max_intervals = std::max(_max_intervals, curCFP.constraints.size());
            auto& arcInterval = _arcIntervals[transitionId][arc.place];
            uint32_t index = 0;
            arcInterval._intervalTupleVec.clear();

            if (!arc.expr->getArcIntervals(arcInterval, curCFP, index, 0)) {
                return false;
            }

            if(_builder.is_partitioned()){
                _builder.partitions()[arc.place].applyPartition(arcInterval);
            }
        }
        return true;
    }

}
