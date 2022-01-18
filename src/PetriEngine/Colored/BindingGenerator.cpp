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
#include "PetriEngine/Colored/ColorOverapprox.h"

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
        _generator->nextBinding();
        if (_generator->isInitial()) _generator = nullptr;
        return *this;
    }

    const Colored::BindingMap& NaiveBindingGenerator::Iterator::operator*() const {
        return _generator->currentBinding();
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
            nextBinding();
    }

    bool NaiveBindingGenerator::eval() const {
        if (_expr == nullptr)
            return true;
        Colored::EquivalenceVec placePartition;

        const Colored::ExpressionContext &context {_bindings, _colorTypes, placePartition};
        return _expr->eval(context);
    }

    const Colored::BindingMap& NaiveBindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            for (auto& binding : _bindings) {
                binding.second = &binding.second->operator++();
                if (binding.second->getId() != 0) {
                    break;
                }
            }

            if (isInitial())
                break;

            test = eval();
        }
        return _bindings;
    }

    const Colored::BindingMap& NaiveBindingGenerator::currentBinding() const {
        return _bindings;
    }

    bool NaiveBindingGenerator::isInitial() const {
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
            _generator->nextBinding();
            if (_generator->_isDone) {
                _generator = nullptr;
            }
        }
        return *this;
    }

    const Colored::BindingMap& FixpointBindingGenerator::Iterator::operator*() const {
        return _generator->currentBinding();
    }

    FixpointBindingGenerator::FixpointBindingGenerator(const Colored::Transition& transition,
        const Colored::ColorTypeMap& colorTypes, const ColorOverapprox::color_map_vector_t& varmaps,  const std::vector<std::set<const Colored::Variable *>>& symmetric_vars)
    : _expr(transition.guard), _colorTypes(colorTypes), _transition(transition), _varmaps(varmaps), _symmetric_vars(symmetric_vars)
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
            generateCombinations((*varSet.begin())->colorType->size()-1, varSet.size(), combinations, temp);
            _symmetric_var_combinations.push_back(combinations);
        }


        for (auto* var : variables) {
            if(_varmaps.empty() || _varmaps[_nextIndex].find(var)->second.empty()){
                _noValidBindings = true;
                break;
            }
            auto color = var->colorType->getColor(_varmaps[_nextIndex].find(var)->second.front().getLowerIds());
            _bindings[var] = color;
        }
        assignSymmetricVars();

        if (!_noValidBindings && !eval())
            nextBinding();
    }

    bool FixpointBindingGenerator::assignSymmetricVars(){
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

    const Colored::BindingMap& FixpointBindingGenerator::nextBinding() {
        bool test = false;
        while (!test) {
            bool next = true;

            if(assignSymmetricVars()){
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

                    const auto &varInterval = _varmaps[_nextIndex].find(binding.first)->second;
                    std::vector<uint32_t> colorIds;
                    binding.second->getTupleId(colorIds);
                    const auto &nextIntervalBinding = varInterval.isRangeEnd(colorIds);

                    if (nextIntervalBinding.size() == 0){
                        binding.second = &binding.second->operator++();
                        _currentInnerId = 0;
                        _currentOuterId = 0;
                        assignSymmetricVars();
                        next = false;
                        break;
                    } else {
                        binding.second = binding.second->getColorType()->getColor(nextIntervalBinding.getLowerIds());
                        _currentInnerId = 0;
                        _currentOuterId = 0;
                        assignSymmetricVars();
                        if(!nextIntervalBinding.equals(varInterval.front())){
                            next = false;

                            break;
                        }
                    }
                }
            }

            if(next){
                _nextIndex++;
                if(isInitial()){
                    _isDone = true;
                    break;
                }
                for(auto& binding : _bindings){
                    binding.second =  binding.second->getColorType()->getColor(_varmaps[_nextIndex].find(binding.first)->second.front().getLowerIds());
                }
            }
            test = eval();
        }

        return _bindings;
    }

    void FixpointBindingGenerator::generateCombinations(
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
            generateCombinations(i, samples - 1, result, current);
            current.pop_back();
        }
    }

    const Colored::BindingMap& FixpointBindingGenerator::currentBinding() const {
        return _bindings;
    }

    bool FixpointBindingGenerator::isInitial() const {
        return _nextIndex >= _varmaps.size();
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