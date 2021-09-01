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

#include "PetriEngine/Colored/BindingGenerator.h"

namespace PetriEngine {

    NaiveBindingGenerator::Iterator::Iterator(NaiveBindingGenerator* generator)
            : _generator(generator)
    {
    }

    bool NaiveBindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool NaiveBindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    NaiveBindingGenerator::Iterator& NaiveBindingGenerator::Iterator::operator++() {
        _generator->next_binding();
        if (_generator->is_initial()) _generator = nullptr;
        return *this;
    }   

    const Colored::BindingMap& NaiveBindingGenerator::Iterator::operator*() const {
        return _generator->current_binding();
    }

    NaiveBindingGenerator::NaiveBindingGenerator(const Colored::Transition& transition,
            Colored::ColorTypeMap& colorTypes)
        : _colorTypes(colorTypes)
    {
        _expr = transition.guard;
        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (const auto& arc : transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (const auto& arc : transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (const auto& var : variables) {
            _bindings[var] = &var->colorType->operator[](0);
        }
        
        if (!eval())
            next_binding();
    }

    bool NaiveBindingGenerator::eval() const {
        if (_expr == nullptr)
            return true;
        Colored::EquivalenceVec placePartition;

        const Colored::ExpressionContext &context {_bindings, _colorTypes, placePartition};
        return _expr->eval(context);
    }

    const Colored::BindingMap& NaiveBindingGenerator::next_binding() {
        bool test = false;
        while (!test) {
            for (auto& binding : _bindings) {
                binding.second = &binding.second->operator++();
                if (binding.second->getId() != 0) {
                    break;
                }
            }

            if (is_initial())
                break;

            test = eval();
        }
        return _bindings;
    }

    const Colored::BindingMap& NaiveBindingGenerator::current_binding() const {
        return _bindings;
    }

    bool NaiveBindingGenerator::is_initial() const {
        for (auto& b : _bindings) {
            if (b.second->getId() != 0) return false;
        }
        return true;
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::begin() {
        return {this};
    }

    NaiveBindingGenerator::Iterator NaiveBindingGenerator::end() {
        return {nullptr};
    }


    FixpointBindingGenerator::Iterator::Iterator(FixpointBindingGenerator* generator)
        : _generator(generator)
    {
    }

    bool FixpointBindingGenerator::Iterator::operator==(Iterator& other) {
        return _generator == other._generator;
    }

    bool FixpointBindingGenerator::Iterator::operator!=(Iterator& other) {
        return _generator != other._generator;
    }

    FixpointBindingGenerator::Iterator& FixpointBindingGenerator::Iterator::operator++() {
        if (_generator->_isDone) {
            _generator = nullptr;
        } else {
            _generator->next_binding();
            if (_generator->_isDone) {
                _generator = nullptr;
            }
        }
        return *this;
    }

    const Colored::BindingMap& FixpointBindingGenerator::Iterator::operator*() const {
        return _generator->current_binding();
    }

    FixpointBindingGenerator::FixpointBindingGenerator(const Colored::Transition& transition,
        const Colored::ColorTypeMap& colorTypes,  const std::vector<std::set<const Colored::Variable *>>& symmetric_vars)
    : _expr(transition.guard), _colorTypes(colorTypes), _transition(transition), _symmetric_vars(symmetric_vars)
    {
        _isDone = false;
        _noValidBindings = false;
        _nextIndex = 0;

        std::set<const Colored::Variable*> variables;
        if (_expr != nullptr) {
            _expr->getVariables(variables);
        }
        for (const auto &arc : _transition.input_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }
        for (const auto &arc : _transition.output_arcs) {
            assert(arc.expr != nullptr);
            arc.expr->getVariables(variables);
        }

        for(const auto &varSet : symmetric_vars){
            std::vector<std::vector<uint32_t>> combinations;
            std::vector<uint32_t> temp;
            generate_combinations(varSet.begin().operator*()->colorType->size()-1, varSet.size(), combinations, temp);
            _symmetric_var_combinations.push_back(combinations);
        }       
        
        
        for (auto* var : variables) {
            if(_transition.variableMaps.empty() || _transition.variableMaps[_nextIndex].find(var)->second.empty()){
                _noValidBindings = true;
                break;
            }
            auto color = var->colorType->getColor(_transition.variableMaps[_nextIndex].find(var)->second.front().getLowerIds());
            _bindings[var] = color;
        }
        assign_symmetric_vars();
        
        if (!_noValidBindings && !eval())
            next_binding();
    }

    bool FixpointBindingGenerator::assign_symmetric_vars(){
        if(_currentOuterId < _symmetric_vars.size()){
            if(_currentInnerId >= _symmetric_var_combinations[_currentOuterId].size()){
                _currentOuterId++;
                _currentInnerId = 0;
            }
        } else {
            return false;
        }
        uint32_t j = 0;
        for(auto var : _symmetric_vars[_currentOuterId]){
            _bindings[var] = &var->colorType->operator[](_symmetric_var_combinations[_currentOuterId][_currentInnerId][j]);
            j++;
        }
        _currentInnerId++;
        if(_currentInnerId >= _symmetric_var_combinations[_currentOuterId].size()){
            if(_currentOuterId < _symmetric_vars.size()){
                _currentOuterId++;
                _currentInnerId = 0;
            } else {
                return false;
            }
        }
        return true;        
    }


    bool FixpointBindingGenerator::eval() const{
        if (_expr == nullptr)
            return true;

        Colored::EquivalenceVec placePartition;
        const Colored::ExpressionContext &context {_bindings, _colorTypes, placePartition};
        return _expr->eval(context);
    }

    const Colored::BindingMap& FixpointBindingGenerator::next_binding() {
        bool test = false;
        while (!test) {
            bool next = true;

            if(assign_symmetric_vars()){
                next = false;
            } else {
                for (auto& binding : _bindings) {
                    bool varSymmetric = false;
                    for(auto& set : _symmetric_vars){
                        if(set.find(binding.first) != set.end()){
                            varSymmetric = true;
                            break;
                        }
                    }
                    if(varSymmetric){
                        continue;
                    }

                    const auto &varInterval = _transition.variableMaps[_nextIndex].find(binding.first)->second;                
                    std::vector<uint32_t> colorIds;
                    binding.second->getTupleId(colorIds);
                    const auto &nextIntervalBinding = varInterval.isRangeEnd(colorIds);

                    if (nextIntervalBinding.size() == 0){                    
                        binding.second = &binding.second->operator++();
                        _currentInnerId = 0;
                        _currentOuterId = 0;
                        assign_symmetric_vars();
                        next = false;
                        break;                    
                    } else {
                        binding.second = binding.second->getColorType()->getColor(nextIntervalBinding.getLowerIds());
                        _currentInnerId = 0;
                        _currentOuterId = 0;
                        assign_symmetric_vars();
                        if(!nextIntervalBinding.equals(varInterval.front())){
                            next = false;
                            
                            break;
                        }              
                    }
                }
            }            

            if(next){
                _nextIndex++;
                if(is_initial()){
                    _isDone = true;
                    break;
                }
                for(auto& binding : _bindings){
                    binding.second =  binding.second->getColorType()->getColor(_transition.variableMaps[_nextIndex].find(binding.first)->second.front().getLowerIds());
                }
            }                 
            test = eval();
        }
        
        return _bindings;
    }

    void FixpointBindingGenerator::generate_combinations(
            uint32_t options,
            uint32_t samples,
            std::vector<std::vector<uint32_t>> &result,
            std::vector<uint32_t> &current) const {
        if (samples == 0) {
            result.push_back(current);
            return;
        }
        for (uint32_t i = 0; i <= options; i++) {
            current.push_back(i);
            generate_combinations(i, samples - 1, result, current);
            current.pop_back();
        }
    }

    const Colored::BindingMap& FixpointBindingGenerator::current_binding() const {
        return _bindings;
    }

    bool FixpointBindingGenerator::is_initial() const {
        return _nextIndex >= _transition.variableMaps.size();
    }

    FixpointBindingGenerator::Iterator FixpointBindingGenerator::begin() {
        if(_noValidBindings || _isDone){
            return {nullptr};
        }
        return {this};
    }

    FixpointBindingGenerator::Iterator FixpointBindingGenerator::end() {
        return {nullptr};
    }
}