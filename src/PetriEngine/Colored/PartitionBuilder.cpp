#include "PetriEngine/Colored/PartitionBuilder.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include <numeric>
#include <chrono>
#include <bits/stl_stack.h>



namespace PetriEngine {
    namespace Colored {

        PartitionBuilder::PartitionBuilder(const ColoredPetriNetBuilder& builder)
        : _builder(builder) {

            //Instantiate partitions
            _partition.resize(_builder.places().size());
            _inQueue.resize(_builder.places().size(), false);
            for(uint32_t i = 0; i < _builder.places().size(); i++){
                const PetriEngine::Colored::Place& place = _builder.places()[i];
                EquivalenceClass fullClass = EquivalenceClass(++_eq_id_counter, place.type);
                if(builder.cfp().computed()){
                    fullClass.setIntervalVector(builder.cfp().places_fixpoint()[i].constraints);
                } else {
                    fullClass.addInterval(place.type->getFullInterval());
                }
                _partition[i].push_back(fullClass);
                for(uint32_t j = 0; j < place.type->productSize(); j++){
                    _partition[i].push_back_diagonalTuplePos(false);
                }
                _placeQueue.push(i);
                _inQueue[i] = true;
            }
        }

        void PartitionBuilder::print() const {
            for(size_t pid = 0; pid < _builder.places().size(); ++pid){
                const auto& place = _builder.places()[pid];
                const auto& equivalenceVec = _partition[pid];
                std::cout << "Partition for place " << place.name << std::endl;
                std::cout << "Diag variables: (";
                for(auto daigPos : equivalenceVec.getDiagonalTuplePositions()){
                    std::cout << daigPos << ",";
                }
                std::cout << ")" << std::endl;
                for (const auto &equivalenceClass : equivalenceVec){
                    std::cout << equivalenceClass.toString() << std::endl;

                }
                std::cout << "Diagonal " << equivalenceVec.isDiagonal() << std::endl << std::endl;;
            }
        }

        bool PartitionBuilder::partition(int32_t timeout) {
            const auto start = std::chrono::high_resolution_clock::now();
            handle_leaf_transitions();
            auto end = std::chrono::high_resolution_clock::now();

            while(!_placeQueue.empty() && timeout > 0 && std::chrono::duration_cast<std::chrono::seconds>(end - start).count() < timeout){
                auto placeId = _placeQueue.top();
                _placeQueue.pop();
                _inQueue[placeId] = false;

                bool allPositionsDiagonal = true;
                for(auto diag : _partition[placeId].getDiagonalTuplePositions()){
                    if(!diag){
                        allPositionsDiagonal = false;
                        break;
                    }
                }

                if(allPositionsDiagonal || _partition[placeId].size() >=
                    _partition[placeId].back().type()->size(_partition[placeId].getDiagonalTuplePositions())){
                    _partition[placeId].setDiagonal(true);
                }

                for(uint32_t transitionId : _builder.place_preset(placeId)){
                    handle_transition(transitionId, placeId);
                }
                end = std::chrono::high_resolution_clock::now();
            }
            auto partitionEnd = std::chrono::high_resolution_clock::now();
            _total_time = (std::chrono::duration_cast<std::chrono::microseconds>(partitionEnd - start).count())*0.000001;
            if(_placeQueue.empty())
            {
                assign_color_map();
                return true;
            }
            else
                return false;
        }

        void PartitionBuilder::assign_color_map() {
            for(size_t pid = 0; pid < _builder.places().size(); ++pid)
            {
                auto& eqVec = _partition[pid];
                if(eqVec.isDiagonal()){
                    continue;
                }

                const auto* colorType = _builder.places()[pid].type;
                for(uint32_t i = 0; i < colorType->size(); i++) {
                    const auto* color = &(*colorType)[i];
                    eqVec.addColorToEqClassMap(color);
                }
            }
        }

        void PartitionBuilder::handle_transition(uint32_t transitionId, uint32_t postPlaceId){
            const auto &transition = _builder.transitions()[transitionId];
            Arc postArc;
            bool arcFound = false;
            for(const auto& outArc : transition.output_arcs){
                if(outArc.place == postPlaceId){
                    postArc = outArc;
                    arcFound = true;
                    break;
                }
            }

            if(!arcFound){
                return;
            }

            handle_transition(transition, postPlaceId, &postArc);
        }

        //Check if a variable appears more than once on the output arc
        //If the place is does not have a product colortype mark the whole place as diagonal, otherwise only the positions
        void PartitionBuilder::check_var_on_arc(const VariableModifierMap &varModifierMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId, bool inputArc){
            for(const auto &varModMap : varModifierMap){
                if(varModMap.second.size() > 1){
                    uint32_t actualSize = 0;
                    std::vector<uint32_t> positions;
                    for(const auto &map : varModMap.second){
                        if(!map.empty()){
                            for(auto position : map){
                                positions.push_back(position.first);
                            }
                            actualSize++;
                        }
                    }
                    if(actualSize > 1) {
                        diagonalVars.insert(varModMap.first);
                        if(_partition[placeId].back().type()->productSize() == 1){
                            _partition[placeId].setDiagonal(true);
                        } else {
                            for(auto pos : positions){
                                if(!_partition[placeId].getDiagonalTuplePositions()[pos]){
                                    if(inputArc) add_to_queue(placeId);
                                    _partition[placeId].setDiagonalTuplePosition(pos,true);
                                }
                            }
                        }
                    }
                }
            }
        }
        //Check if the variales on this preArc also appear on other preArcs for the transition
        //and mark them as diagonal if they do
        void PartitionBuilder::check_var_on_input_arcs(const std::unordered_map<uint32_t,PositionVariableMap> &placeVariableMap, const PositionVariableMap &preVarPositionMap, std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId){
            for(const auto &placeVariables : placeVariableMap){
                for(const auto &variable : preVarPositionMap){
                    for(const auto &varPosition : placeVariables.second){
                        if(varPosition.second == variable.second){
                            diagonalVars.insert(variable.second);
                            if(_partition[placeId].back().type()->productSize() == 1){
                                _partition[placeId].setDiagonal(true);
                            } else if(!_partition[placeId].getDiagonalTuplePositions()[variable.first]) {
                                add_to_queue(placeId);
                                _partition[placeId].setDiagonalTuplePosition(variable.first,  true);
                            }

                            if(_partition[placeVariables.first].back().type()->productSize() == 1){
                                _partition[placeVariables.first].setDiagonal(true);
                                add_to_queue(placeVariables.first);
                            } else if(!_partition[placeVariables.first].getDiagonalTuplePositions()[varPosition.first]) {
                                add_to_queue(placeVariables.first);
                                _partition[placeVariables.first].setDiagonalTuplePosition(varPosition.first, true);
                            }
                            break;
                        }
                    }
                    if(_partition[placeId].isDiagonal()){
                        break;
                    }
                }
                if(_partition[placeId].isDiagonal()){
                    break;
                }
            }
        }
        //Check if the preArc share variables with the postArc and mark diagonal if the
        //variable positions are diagonal in the post place
        void PartitionBuilder::mark_shared_vars(const PositionVariableMap &preVarPositionMap, const PositionVariableMap &varPositionMap, uint32_t postPlaceId, uint32_t prePlaceId){
            for(const auto &preVar : preVarPositionMap){
                for(const auto &postVar : varPositionMap){
                    if(preVar.second == postVar.second){
                        if(_partition[postPlaceId].isDiagonal() || _partition[postPlaceId].getDiagonalTuplePositions()[postVar.first]){
                            if(_partition[prePlaceId].back().type()->productSize() == 1){
                                _partition[prePlaceId].setDiagonal(true);
                            } else if(!_partition[prePlaceId].getDiagonalTuplePositions()[preVar.first]) {
                                add_to_queue(prePlaceId);
                                _partition[prePlaceId].setDiagonalTuplePosition(preVar.first, true);
                            }
                        }
                    }
                }
            }
        }
        //Check if any of the variables on the preArc was part of a diagonal constraint in the gaurd
        void PartitionBuilder::check_var_in_guard(const PositionVariableMap &preVarPositionMap, const std::set<const Colored::Variable*> &diagonalVars, uint32_t placeId){
            for(const auto &preVar : preVarPositionMap){
                if(diagonalVars.count(preVar.second)){
                    if(_partition[placeId].back().type()->productSize() == 1){
                        _partition[placeId].setDiagonal(true);
                        break;
                    } else if(!_partition[placeId].getDiagonalTuplePositions()[preVar.first]) {
                        add_to_queue(placeId);
                        _partition[placeId].setDiagonalTuplePosition(preVar.first, true);
                    }
                }
            }
        }

        //Check if we have marked all positions in the product type of the place as diagonal
        //and mark the whole place as diagonal if it is the case
        bool PartitionBuilder::check_tuple_diagonal(uint32_t placeId){
            bool allPositionsDiagonal = true;
            for(auto diag : _partition[placeId].getDiagonalTuplePositions()){
                if(!diag){
                    allPositionsDiagonal = false;
                    break;
                }
            }

            if(allPositionsDiagonal){
                _partition[placeId].setDiagonal(true);
                add_to_queue(placeId);
                return true;
            }
            return false;
        }

        bool PartitionBuilder::check_diagonal(uint32_t placeId){
            if(_partition[placeId].isDiagonal()){
                add_to_queue(placeId);
                return true;
            }
            return false;
        }

        void PartitionBuilder::apply_new_intervals(const Arc &inArc, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps){
            //Retrieve the intervals for the current place,
            //based on the intervals from the postPlace, the postArc, preArc and guard
            auto outIntervals = inArc.expr->getOutputIntervals(varMaps);
            EquivalenceVec newEqVec;
            for(auto& intervalTuple : outIntervals){
                intervalTuple.simplify();
                EquivalenceClass newEqClass(++_eq_id_counter, _partition[inArc.place].back().type(), std::move(intervalTuple));
                newEqVec.push_back(std::move(newEqClass));
            }
            newEqVec.setDiagonalTuplePositions(_partition[inArc.place].getDiagonalTuplePositions());

            //If the prePlace has not been marked as diagonal, then split the current partitions based on the new intervals
            if(split_partition(std::move(newEqVec), inArc.place)){
                add_to_queue(inArc.place);
            }
            _partition[inArc.place].mergeEqClasses();
        }

        void PartitionBuilder::handle_transition(const Transition &transition, const uint32_t postPlaceId, const Arc *postArc) {
            VariableModifierMap varModifierMap;
            PositionVariableMap varPositionMap;
            std::set<const PetriEngine::Colored::Variable *> postArcVars;
            std::set<const PetriEngine::Colored::Variable *> guardVars;
            std::set<const Colored::Variable*> diagonalVars;

            postArc->expr->getVariables(postArcVars, varPositionMap, varModifierMap, true);

            check_var_on_arc(varModifierMap, diagonalVars, postPlaceId, false);

            if(transition.guard != nullptr){
                transition.guard->getVariables(guardVars);
            }
            // we have to copy here, the following loop has the *potential* to modify _partition[postPlaceId]
            const auto placePartition = _partition[postPlaceId].get_classes();

            //Partition each of the equivalence classes
            for(const auto &eqClass : placePartition){
                auto varMaps = prepare_variables(varModifierMap, eqClass, postArc, postPlaceId);

                //If there are variables in the guard, that doesn't come from the postPlace
                //we give them the full interval
                for(auto& varMap : varMaps){
                    for(auto* var : guardVars){
                        if(varMap.count(var) == 0){
                            varMap[var].addInterval(var->colorType->getFullInterval());
                        }
                    }
                }
                if(transition.guard != nullptr){
                    transition.guard->restrictVars(varMaps, diagonalVars);
                }

                handle_in_arcs(transition, diagonalVars, varPositionMap, varMaps, postPlaceId);
            }
        }

        void PartitionBuilder::handle_in_arcs(const Transition &transition, std::set<const Colored::Variable*> &diagonalVars, const PositionVariableMap &varPositionMap, const std::vector<PetriEngine::Colored::VariableIntervalMap> &varMaps, uint32_t postPlaceId){
            std::unordered_map<uint32_t,PositionVariableMap> placeVariableMap;
            for(const auto &inArc : transition.input_arcs){
                //Hack to avoid considering dot places
                if(_builder.places()[inArc.place].type == ColorType::dotInstance()){
                    _partition[inArc.place].setDiagonal(true);
                }

                if(_partition[inArc.place].isDiagonal()){
                    continue;
                }
                VariableModifierMap preVarModifierMap;
                PositionVariableMap preVarPositionMap;
                std::set<const PetriEngine::Colored::Variable *> preArcVars;
                inArc.expr->getVariables(preArcVars, preVarPositionMap, preVarModifierMap, true);
                check_var_on_input_arcs(placeVariableMap, preVarPositionMap, diagonalVars, inArc.place);
                placeVariableMap[inArc.place] = preVarPositionMap;
                if(check_diagonal(inArc.place)) continue;

                mark_shared_vars(preVarPositionMap, varPositionMap, postPlaceId, inArc.place);
                if(check_diagonal(inArc.place)) continue;
                check_var_on_arc(preVarModifierMap, diagonalVars, inArc.place, true);
                if(check_diagonal(inArc.place)) continue;

                check_var_in_guard(preVarPositionMap, diagonalVars, inArc.place);
                if(check_diagonal(inArc.place)) continue;

                _partition[inArc.place].mergeEqClasses();
                if(_partition[inArc.place].size() >=
                    _partition[inArc.place].back().type()->size(_partition[inArc.place].getDiagonalTuplePositions())){
                    _partition[inArc.place].setDiagonal(true);
                    continue;
                }

                if(check_tuple_diagonal(inArc.place)){
                    continue;
                }

                apply_new_intervals(inArc, varMaps);
            }
        }

        void PartitionBuilder::add_to_queue(uint32_t placeId){
            if(!_inQueue[placeId]){
                _placeQueue.push(placeId);
                _inQueue[placeId] = true;
            }
        }


        bool PartitionBuilder::split_partition(PetriEngine::Colored::EquivalenceVec equivalenceVec, uint32_t placeId){
            bool split = false;
            if(_partition[placeId].empty()){
                _partition[placeId] = equivalenceVec;
            } else {
                EquivalenceClass intersection(++_eq_id_counter);
                uint32_t ecPos1 = 0, ecPos2 = 0;
                while(find_overlap(equivalenceVec, _partition[placeId],ecPos1, ecPos2, intersection)) {
                    const auto &ec1 = equivalenceVec[ecPos1];
                    const auto &ec2 = _partition[placeId][ecPos2];
                    const auto rightSubtractEc = ec1.subtract(++_eq_id_counter, ec2, equivalenceVec.getDiagonalTuplePositions());
                    const auto leftSubtractEc = ec2.subtract(++_eq_id_counter, ec1, _partition[placeId].getDiagonalTuplePositions());

                    equivalenceVec.erase(ecPos1);
                    _partition[placeId].erase(ecPos2);

                    if(!intersection.empty()){
                        _partition[placeId].push_back(intersection);
                        intersection.clear();
                    }
                    if(!leftSubtractEc.empty()){
                        _partition[placeId].push_back(leftSubtractEc);
                        split = true;
                    }
                    if(!rightSubtractEc.empty()){
                        equivalenceVec.push_back(rightSubtractEc);
                    }
                }
            }
            return split;
        }

        bool PartitionBuilder::find_overlap(const EquivalenceVec &equivalenceVec1, const EquivalenceVec &equivalenceVec2, uint32_t &overlap1, uint32_t &overlap2, EquivalenceClass &intersection) {
            for(uint32_t i = 0; i < equivalenceVec1.size(); i++){
                for(uint32_t j = 0; j < equivalenceVec2.size(); j++){
                    const auto &ec = equivalenceVec1[i];
                    const auto &ec2 = equivalenceVec2[j];

                    auto intersectingEc = ec.intersect(++_eq_id_counter, ec2);
                    if(!intersectingEc.empty()){
                        overlap1 = i;
                        overlap2 = j;
                        intersection = intersectingEc;
                        return true;
                    }
                }
            }
            return false;
        }

        std::vector<VariableIntervalMap>
        PartitionBuilder::prepare_variables(
                    const VariableModifierMap &varModifierMap,
                    const EquivalenceClass& eqClass , const Arc *arc, uint32_t placeId){
            std::vector<VariableIntervalMap> varMaps;
            VariableIntervalMap varMap;
            varMaps.push_back(varMap);
            std::unordered_map<uint32_t, ArcIntervals> placeArcIntervals;
            ColorFixpoint postPlaceFixpoint;
            postPlaceFixpoint.constraints = eqClass.intervals();
            ArcIntervals newArcInterval(&postPlaceFixpoint, varModifierMap);
            uint32_t index = 0;

            arc->expr->getArcIntervals(newArcInterval, postPlaceFixpoint, index, 0);
            placeArcIntervals[placeId] = std::move(newArcInterval);
            _interval_generator.getVarIntervals(varMaps, placeArcIntervals);

            return varMaps;
        }

        void PartitionBuilder::handle_leaf_transitions(){
            for(uint32_t i = 0; i < _builder.transitions().size(); ++i){
                const auto &transition = _builder.transitions()[i];
                if(transition.output_arcs.empty() && !transition.input_arcs.empty()){
                    handle_transition(transition, transition.input_arcs.back().place, &transition.input_arcs.back());
                }
            }
        }
    }
}
