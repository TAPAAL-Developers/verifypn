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

#include "PetriEngine/Colored/SymmetryDetector.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"

namespace PetriEngine {

    SymmetryDetector::SymmetryDetector(const ColoredPetriNetBuilder& builder)
    : _builder(builder) {
        _symmetric.resize(_builder.transitions().size());

    }

    void SymmetryDetector::compute() {
        if (_builder.isColored()) {
            for (uint32_t transitionId = 0; transitionId < _builder.transitions().size(); ++transitionId) {
                const auto &transition = _builder.transitions()[transitionId];
                if (transition.guard) {
                    continue;
                    //the variables cannot appear on the guard
                }

                for (const auto &inArc : transition.input_arcs) {
                    std::set<const Colored::Variable*> inArcVars;
                    std::vector<uint32_t> numbers;

                    //Application of symmetric variables for partitioned places is currently unhandled
                    if (_builder.is_partitioned() && !_builder.partitions()[inArc.place].isDiagonal()) {
                        continue;
                    }

                    //the expressions is eligible if it is an addexpression that contains only
                    //numberOfExpressions with the same number
                    bool isEligible = inArc.expr->isEligibleForSymmetry(numbers);

                    if (isEligible && numbers.size() > 1) {
                        inArc.expr->getVariables(inArcVars);
                        //It cannot be symmetric with anything if there is only one variable
                        if (inArcVars.size() < 2) {
                            continue;
                        }
                        //The variables may appear only on one input arc and one output arc
                        isEligible = check_in_arcs(transition, inArc, inArcVars);


                        //All the variables have to appear on exactly one output arc and nowhere else
                        if (isEligible)
                            isEligible = check_out_arcs(transition, inArcVars);
                    } else {
                        isEligible = false;
                    }
                    if (isEligible) {
                        _symmetric[transitionId].emplace_back(inArcVars);
                    }
                }
            }
        } else {
            throw base_error("ERROR: Called variable symmetry detection on non-colored net");
        }
    }

    bool SymmetryDetector::check_in_arcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars) const {
        for (auto& otherInArc : transition.input_arcs) {
            if (inArc.place == otherInArc.place) {
                continue;
            }
            std::set<const Colored::Variable*> otherArcVars;
            otherInArc.expr->getVariables(otherArcVars);
            for (auto* var : inArcVars) {
                if (otherArcVars.find(var) != otherArcVars.end()) {
                    return false;
                }
            }
        }
        return true;
    }

    bool SymmetryDetector::check_out_arcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars) const {
        uint32_t numArcs = 0;
        bool foundSomeVars = false;
        for (auto& outputArc : transition.output_arcs) {
            bool foundArc = true;
            std::set<const Colored::Variable*> otherArcVars;
            outputArc.expr->getVariables(otherArcVars);
            for (auto* var : inArcVars) {
                if (otherArcVars.find(var) == otherArcVars.end()) {
                    foundArc = false;
                } else {
                    foundSomeVars = true;
                }
            }
            if (foundArc) {
                //Application of symmetric variables for partitioned places is currently unhandled
                if (_builder.is_partitioned() && !_builder.partitions()[outputArc.place].isDiagonal()) {
                    return false;
                }
                numArcs++;
                //All vars were present
                foundSomeVars = false;
            }
            //If some vars are present the vars are not eligible
            if (foundSomeVars) {
                return false;
            }
        }

        if (numArcs != 1)
            return false;
    }

    void SymmetryDetector::print() const {
        for (uint32_t transitionId = 0; transitionId < _builder.transitions().size(); ++transitionId) {
            const auto &transition = _builder.transitions()[transitionId];

            std::cout << "Transition " << transition.name << " has symmetric variables: " << std::endl;
            for (const auto &set : _symmetric[transitionId]) {
                std::string toPrint = "SET: ";
                for (auto* variable : set) {
                    toPrint += variable->name + ", ";
                }
                std::cout << toPrint << std::endl;
            }
        }
    }


}