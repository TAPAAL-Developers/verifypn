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

#ifndef COLORED_EXPRESSIONS_H
#define COLORED_EXPRESSIONS_H

#include <cassert>
#include <iostream>
#include <memory>
#include <set>
#include <stdlib.h>
#include <string>
#include <typeinfo>
#include <unordered_map>

#include "ArcIntervals.h"
#include "Colors.h"
#include "EquivalenceVec.h"
#include "GuardRestrictor.h"
#include "Multiset.h"
#include "errorcodes.h"

namespace PetriEngine {
class ColoredPetriNetBuilder;

namespace Colored {
struct ExpressionContext {

    const BindingMap &_binding;
    const ColorTypeMap &_colorTypes;
    const Colored::EquivalenceVec &_placePartition;

    const Color *find_color(const std::string &color) const {
        for (auto &elem : _colorTypes) {
            auto col = (*elem.second)[color];
            if (col)
                return col;
        }
        throw base_error("Could not find color: ", color, "\nCANNOT_COMPUTE\n");
    }

    const ProductType *find_product_color_type(const std::vector<const ColorType *> &types) const {
        for (auto &elem : _colorTypes) {
            auto *pt = dynamic_cast<const ProductType *>(elem.second);

            if (pt && pt->contains_types(types)) {
                return pt;
            }
        }
        return nullptr;
    }
};

class WeightException : public base_error {
  public:
    explicit WeightException(std::string message) : base_error("Undefinded weight: ", message) {}
};

template <typename Base, typename T> inline bool instanceof (const T *) {
    return std::is_base_of<Base, T>::value;
}

class Expression {
  public:
    Expression() {}

    virtual void get_variables(std::set<const Colored::Variable *> &variables,
                               PositionVariableMap &varPositions,
                               VariableModifierMap &varModifierMap, bool includeSubtracts,
                               uint32_t &index) const {}

    virtual void get_variables(std::set<const Colored::Variable *> &variables,
                               PositionVariableMap &varPositions,
                               VariableModifierMap &varModifierMap, bool includeSubtracts) const {
        uint32_t index = 0;
        get_variables(variables, varPositions, varModifierMap, includeSubtracts, index);
    }

    virtual void get_variables(std::set<const Colored::Variable *> &variables,
                               PositionVariableMap &varPositions) const {
        VariableModifierMap varModifierMap;
        uint32_t index = 0;
        get_variables(variables, varPositions, varModifierMap, false, index);
    }

    virtual void get_variables(std::set<const Colored::Variable *> &variables) const {
        PositionVariableMap varPositions;
        VariableModifierMap varModifierMap;
        uint32_t index = 0;

        get_variables(variables, varPositions, varModifierMap, false, index);
    }

    virtual bool is_tuple() const { return false; }

    virtual bool is_eligible_for_symmetry(std::vector<uint32_t> &numbers) const { return false; }

    virtual std::string to_string() const { return "Unsupported"; }
};

class ColorExpression : public Expression {
  public:
    ColorExpression() {}
    virtual ~ColorExpression() {}

    virtual const Color *eval(const ExpressionContext &context) const = 0;

    virtual void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                               uint32_t &index) const = 0;

    virtual bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                                   const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                                   int32_t modifier) const = 0;

    virtual const ColorType *get_color_type(const ColorTypeMap &colorTypes) const = 0;

    virtual Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const {
        return Colored::interval_vector_t();
    }
};

class DotConstantExpression : public ColorExpression {
  public:
    const Color *eval(const ExpressionContext &context) const override {
        return &(*ColorType::dot_instance()->begin());
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        if (arcIntervals._intervalTupleVec.empty()) {
            // We can add all place tokens when considering the dot constant as, that must be
            // present
            arcIntervals._intervalTupleVec.push_back(cfp._constraints);
        }
        return !cfp._constraints.empty();
    }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        Colored::interval_t interval;
        Colored::interval_vector_t tupleInterval;
        const Color *dotColor = &(*ColorType::dot_instance()->begin());

        colortypes.emplace_back(dotColor->get_color_type());

        interval.add_range(dotColor->get_id(), dotColor->get_id());
        tupleInterval.add_interval(interval);
        return tupleInterval;
    }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {
        const Color *dotColor = &(*ColorType::dot_instance()->begin());
        constantMap[index] = dotColor;
    }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {
        return ColorType::dot_instance();
    }

    std::string to_string() const override { return "dot"; }
};

typedef std::shared_ptr<ColorExpression> ColorExpression_ptr;

class VariableExpression : public ColorExpression {
  private:
    const Variable *_variable;

  public:
    const Color *eval(const ExpressionContext &context) const override {
        return context._binding.find(_variable)->second;
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        variables.insert(_variable);
        varPositions[index] = _variable;
        if (varModifierMap.count(_variable) == 0) {
            std::vector<std::unordered_map<uint32_t, int32_t>> newVec;

            for (auto pair : varModifierMap) {
                for (uint32_t i = 0; i < pair.second.size() - 1; i++) {
                    std::unordered_map<uint32_t, int32_t> emptyMap;
                    newVec.push_back(emptyMap);
                }
                break;
            }
            std::unordered_map<uint32_t, int32_t> newMap;
            newMap[index] = 0;
            newVec.push_back(newMap);
            varModifierMap[_variable] = newVec;
        } else {
            varModifierMap[_variable].back()[index] = 0;
        }
    }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        Colored::interval_vector_t varInterval;

        // If we see a new variable on an out arc, it gets its full interval
        if (varMap.count(_variable) == 0) {
            varInterval.add_interval(_variable->_colorType->get_full_interval());
        } else {
            for (const auto &interval : varMap.find(_variable)->second) {
                varInterval.add_interval(interval);
            }
        }

        std::vector<const ColorType *> varColorTypes;
        _variable->_colorType->get_colortypes(varColorTypes);

        for (auto &ct : varColorTypes) {
            colortypes.push_back(std::move(ct));
        }

        return varInterval;
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        if (arcIntervals._intervalTupleVec.empty()) {
            // As variables does not restrict the values before the guard we include all tokens
            arcIntervals._intervalTupleVec.push_back(cfp._constraints);
        }
        return !cfp._constraints.empty();
    }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {}

    std::string to_string() const override { return _variable->_name; }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {
        return _variable->_colorType;
    }

    VariableExpression(const Variable *variable) : _variable(variable) {}
};

class UserOperatorExpression : public ColorExpression {
  private:
    const Color *_userOperator;

  public:
    const Color *eval(const ExpressionContext &context) const override {
        if (context._placePartition.get_eq_classes().empty()) {
            return _userOperator;
        } else {
            std::vector<uint32_t> tupleIds;
            _userOperator->get_tuple_id(tupleIds);

            context._placePartition.apply_partition(tupleIds);
            return _userOperator->get_color_type()->get_color(tupleIds);
        }
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        uint32_t colorId = _userOperator->get_id() + modifier;
        while (colorId < 0) {
            colorId += _userOperator->get_color_type()->size();
        }
        colorId = colorId % _userOperator->get_color_type()->size();

        if (arcIntervals._intervalTupleVec.empty()) {
            Colored::interval_vector_t newIntervalTuple;
            bool colorInFixpoint = false;
            for (const auto &interval : cfp._constraints) {
                if (interval[index].contains(colorId)) {
                    newIntervalTuple.add_interval(interval);
                    colorInFixpoint = true;
                }
            }
            arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
            return colorInFixpoint;
        } else {
            std::vector<uint32_t> intervalsToRemove;
            for (auto &intervalTuple : arcIntervals._intervalTupleVec) {
                for (uint32_t i = 0; i < intervalTuple.size(); i++) {
                    if (!intervalTuple[i][index].contains(colorId)) {
                        intervalsToRemove.push_back(i);
                    }
                }

                for (auto i = intervalsToRemove.rbegin(); i != intervalsToRemove.rend(); ++i) {
                    intervalTuple.remove_interval(*i);
                }
            }
            return !arcIntervals._intervalTupleVec[0].empty();
        }
    }

    std::string to_string() const override { return _userOperator->to_string(); }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {
        constantMap[index] = _userOperator;
    }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {
        return _userOperator->get_color_type();
    }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        Colored::interval_t interval;
        Colored::interval_vector_t tupleInterval;

        colortypes.emplace_back(_userOperator->get_color_type());

        interval.add_range(_userOperator->get_id(), _userOperator->get_id());
        tupleInterval.add_interval(interval);
        return tupleInterval;
    }

    UserOperatorExpression(const Color *userOperator) : _userOperator(userOperator) {}
};

class SuccessorExpression : public ColorExpression {
  private:
    ColorExpression_ptr _color;

  public:
    const Color *eval(const ExpressionContext &context) const override {
        return &++(*_color->eval(context));
    }

    bool is_tuple() const override { return _color->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        // save index before evaluating nested expression to decrease all the correct modifiers
        uint32_t indexBefore = index;
        _color->get_variables(variables, varPositions, varModifierMap, includeSubtracts, index);
        for (auto &varModifierPair : varModifierMap) {
            for (auto &idModPair : varModifierPair.second.back()) {
                if (idModPair.first <= index && idModPair.first >= indexBefore) {
                    idModPair.second--;
                }
            }
        }
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        return _color->get_arc_intervals(arcIntervals, cfp, index, modifier + 1);
    }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        // store the number of colortyps already in colortypes vector and use that as offset when
        // indexing it
        auto colortypesBefore = colortypes.size();

        auto nestedInterval = _color->get_output_intervals(varMap, colortypes);
        Colored::GuardRestrictor guardRestrictor = Colored::GuardRestrictor();
        return guardRestrictor.shift_intervals(varMap, colortypes, nestedInterval, 1,
                                               colortypesBefore);
    }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {
        _color->get_constants(constantMap, index);
        for (auto &constIndexPair : constantMap) {
            constIndexPair.second = &constIndexPair.second->operator++();
        }
    }

    std::string to_string() const override { return _color->to_string() + "++"; }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {
        return _color->get_color_type(colorTypes);
    }

    SuccessorExpression(ColorExpression_ptr &&color) : _color(std::move(color)) {}
};

class PredecessorExpression : public ColorExpression {
  private:
    ColorExpression_ptr _color;

  public:
    const Color *eval(const ExpressionContext &context) const override {
        return &--(*_color->eval(context));
    }

    bool is_tuple() const override { return _color->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        // save index before evaluating nested expression to decrease all the correct modifiers
        uint32_t indexBefore = index;
        _color->get_variables(variables, varPositions, varModifierMap, includeSubtracts, index);
        for (auto &varModifierPair : varModifierMap) {
            for (auto &idModPair : varModifierPair.second.back()) {
                if (idModPair.first <= index && idModPair.first >= indexBefore) {
                    idModPair.second++;
                }
            }
        }
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        return _color->get_arc_intervals(arcIntervals, cfp, index, modifier - 1);
    }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        // store the number of colortyps already in colortypes vector and use that as offset when
        // indexing it
        auto colortypesBefore = colortypes.size();

        auto nestedInterval = _color->get_output_intervals(varMap, colortypes);
        Colored::GuardRestrictor guardRestrictor;
        return guardRestrictor.shift_intervals(varMap, colortypes, nestedInterval, -1,
                                               colortypesBefore);
    }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {
        _color->get_constants(constantMap, index);
        for (auto &constIndexPair : constantMap) {
            constIndexPair.second = &constIndexPair.second->operator--();
        }
    }

    std::string to_string() const override { return _color->to_string() + "--"; }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {
        return _color->get_color_type(colorTypes);
    }

    PredecessorExpression(ColorExpression_ptr &&color) : _color(std::move(color)) {}
};

class TupleExpression : public ColorExpression {
  private:
    std::vector<ColorExpression_ptr> _colors;
    const ColorType *_colorType = nullptr;

  public:
    const Color *eval(const ExpressionContext &context) const override {
        std::vector<const Color *> colors;
        std::vector<const ColorType *> types;
        for (const auto &color : _colors) {
            colors.push_back(color->eval(context));
            types.push_back(colors.back()->get_color_type());
        }
        const ProductType *pt = context.find_product_color_type(types);
        assert(pt != nullptr);
        const Color *col = pt->get_color(colors);
        assert(col != nullptr);
        return col;
    }

    bool is_tuple() const override { return true; }

    Colored::interval_vector_t
    get_output_intervals(const VariableIntervalMap &varMap,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        Colored::interval_vector_t intervals;

        for (const auto &colorExp : _colors) {
            Colored::interval_vector_t intervalHolder;
            auto nested_intervals = colorExp->get_output_intervals(varMap, colortypes);

            if (intervals.empty()) {
                intervals = nested_intervals;
            } else {
                for (const auto &nested_interval : nested_intervals) {
                    Colored::interval_vector_t newIntervals;
                    for (auto interval : intervals) {
                        for (const auto &nestedRange : nested_interval._ranges) {
                            interval.add_range(nestedRange);
                        }
                        newIntervals.add_interval(interval);
                    }
                    for (const auto &newInterval : newIntervals) {
                        intervalHolder.add_interval(newInterval);
                    }
                }
                intervals = intervalHolder;
            }
        }
        return intervals;
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        for (const auto &expr : _colors) {
            bool res = expr->get_arc_intervals(arcIntervals, cfp, index, modifier);
            if (!res) {
                return false;
            }
            ++index;
        }
        return true;
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        for (const auto &elem : _colors) {
            elem->get_variables(variables, varPositions, varModifierMap, includeSubtracts, index);
            ++index;
        }
    }

    const ColorType *get_color_type(const ColorTypeMap &colorTypes) const override {

        std::vector<const ColorType *> types;
        if (_colorType != nullptr) {
            return _colorType;
        }

        for (const auto &color : _colors) {
            types.push_back(color->get_color_type(colorTypes));
        }

        for (auto elem : colorTypes) {
            auto *pt = dynamic_cast<const ProductType *>(elem.second);
            if (pt && pt->contains_types(types)) {
                return pt;
            }
        }
        std::cout << "COULD NOT FIND PRODUCT TYPE" << std::endl;
        assert(false);
        return nullptr;
    }

    void get_constants(std::unordered_map<uint32_t, const Color *> &constantMap,
                       uint32_t &index) const override {
        for (const auto &elem : _colors) {
            elem->get_constants(constantMap, index);
            index++;
        }
    }

    std::string to_string() const override {
        std::string res = "(" + _colors[0]->to_string();
        for (uint32_t i = 1; i < _colors.size(); ++i) {
            res += "," + _colors[i]->to_string();
        }
        res += ")";
        return res;
    }

    void set_color_type(const ColorType *ct) { _colorType = ct; }

    TupleExpression(std::vector<ColorExpression_ptr> &&colors) : _colors(std::move(colors)) {}
};

class GuardExpression : public Expression {
  public:
    GuardExpression() {}
    virtual ~GuardExpression() {}

    virtual bool eval(const ExpressionContext &context) const = 0;

    virtual void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                               std::set<const Colored::Variable *> &diagonalVars) const = 0;

    virtual void restrict_vars(std::vector<VariableIntervalMap> &variableMap) const {
        std::set<const Colored::Variable *> diagonalVars;
        restrict_vars(variableMap, diagonalVars);
    }
};

typedef std::shared_ptr<GuardExpression> GuardExpression_ptr;

class LessThanExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) < _right->eval(context);
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Variable *> leftVars;
        std::set<const Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }
        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_vars(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                      varPositionsR, constantMapL, constantMapR, diagonalVars, true,
                                      true);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " < " + _right->to_string();
        return res;
    }

    LessThanExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class GreaterThanExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) > _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Colored::Variable *> leftVars;
        std::set<const Colored::Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }

        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_vars(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                      varPositionsR, constantMapL, constantMapR, diagonalVars,
                                      false, true);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " > " + _right->to_string();
        return res;
    }

    GreaterThanExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class LessThanEqExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) <= _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Colored::Variable *> leftVars;
        std::set<const Colored::Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }

        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_vars(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                      varPositionsR, constantMapL, constantMapR, diagonalVars, true,
                                      false);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " <= " + _right->to_string();
        return res;
    }

    LessThanEqExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class GreaterThanEqExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) >= _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Colored::Variable *> leftVars;
        std::set<const Colored::Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }

        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_vars(variableMap, varModifierMapL, varModifierMapR, varPositionsL,
                                      varPositionsR, constantMapL, constantMapR, diagonalVars,
                                      false, false);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " >= " + _right->to_string();
        return res;
    }

    GreaterThanEqExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class EqualityExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) == _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Colored::Variable *> leftVars;
        std::set<const Colored::Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }

        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_equality(variableMap, varModifierMapL, varModifierMapR,
                                          varPositionsL, varPositionsR, constantMapL, constantMapR,
                                          diagonalVars);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " == " + _right->to_string();
        return res;
    }

    EqualityExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class InequalityExpression : public GuardExpression {
  private:
    ColorExpression_ptr _left;
    ColorExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) != _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        VariableModifierMap varModifierMapL;
        VariableModifierMap varModifierMapR;
        PositionVariableMap varPositionsL;
        PositionVariableMap varPositionsR;
        std::unordered_map<uint32_t, const Color *> constantMapL;
        std::unordered_map<uint32_t, const Color *> constantMapR;
        std::set<const Colored::Variable *> leftVars;
        std::set<const Colored::Variable *> rightVars;
        uint32_t index = 0;
        _left->get_variables(leftVars, varPositionsL, varModifierMapL, false);
        _right->get_variables(rightVars, varPositionsR, varModifierMapR, false);
        _left->get_constants(constantMapL, index);
        index = 0;
        _right->get_constants(constantMapR, index);

        if (leftVars.empty() && rightVars.empty()) {
            return;
        }
        Colored::GuardRestrictor guardRestrictor;
        guardRestrictor.restrict_inequality(variableMap, varModifierMapL, varModifierMapR,
                                            varPositionsL, varPositionsR, constantMapL,
                                            constantMapR, diagonalVars);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " != " + _right->to_string();
        return res;
    }

    InequalityExpression(ColorExpression_ptr &&left, ColorExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class AndExpression : public GuardExpression {
  private:
    GuardExpression_ptr _left;
    GuardExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) && _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        _left->restrict_vars(variableMap, diagonalVars);
        _right->restrict_vars(variableMap, diagonalVars);
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " && " + _right->to_string();
        return res;
    }

    AndExpression(GuardExpression_ptr &&left, GuardExpression_ptr &&right)
        : _left(left), _right(right) {}
};

class OrExpression : public GuardExpression {
  private:
    GuardExpression_ptr _left;
    GuardExpression_ptr _right;

  public:
    bool eval(const ExpressionContext &context) const override {
        return _left->eval(context) || _right->eval(context);
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    void restrict_vars(std::vector<VariableIntervalMap> &variableMap,
                       std::set<const Colored::Variable *> &diagonalVars) const override {
        auto varMapCopy = variableMap;
        _left->restrict_vars(variableMap, diagonalVars);
        _right->restrict_vars(varMapCopy, diagonalVars);

        variableMap.insert(variableMap.end(), varMapCopy.begin(), varMapCopy.end());
    }

    std::string to_string() const override {
        std::string res = _left->to_string() + " || " + _right->to_string();
        return res;
    }

    OrExpression(GuardExpression_ptr &&left, GuardExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class ArcExpression : public Expression {
  public:
    ArcExpression() {}
    virtual ~ArcExpression() {}

    virtual Multiset eval(const ExpressionContext &context) const = 0;

    virtual void get_constants(PositionColorsMap &constantMap, uint32_t &index) const = 0;

    virtual bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                                   const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                                   int32_t modifier) const = 0;

    virtual uint32_t weight() const = 0;

    virtual std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec) const {
        std::vector<const Colored::ColorType *> colortypes;

        return get_output_intervals(varMapVec, colortypes);
    }

    virtual std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const {
        return std::vector<Colored::interval_vector_t>(0);
    }
};

typedef std::shared_ptr<ArcExpression> ArcExpression_ptr;

class AllExpression : public Expression {
  private:
    const ColorType *_sort;

  public:
    virtual ~AllExpression(){};
    std::vector<std::pair<const Color *, uint32_t>> eval(const ExpressionContext &context) const {
        std::vector<std::pair<const Color *, uint32_t>> colors;
        assert(_sort != nullptr);
        if (context._placePartition.is_diagonal() ||
            context._placePartition.get_eq_classes().empty()) {
            for (size_t i = 0; i < _sort->size(); i++) {
                colors.push_back(std::make_pair(&(*_sort)[i], 1));
            }
        } else {
            for (const auto &eq_class : context._placePartition.get_eq_classes()) {
                colors.push_back(std::make_pair(
                    _sort->get_color(eq_class.intervals().get_lower_ids()), eq_class.size()));
            }
        }

        return colors;
    }

    bool is_tuple() const override { return _sort->product_size() > 1; }

    void get_constants(PositionColorsMap &constantMap, uint32_t &index) const {
        for (size_t i = 0; i < _sort->size(); i++) {
            constantMap[index].push_back(&(*_sort)[i]);
        }
    }

    Colored::interval_vector_t
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const {
        Colored::interval_vector_t newIntervalTuple;
        newIntervalTuple.add_interval(_sort->get_full_interval());
        return newIntervalTuple;
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const {

        if (arcIntervals._intervalTupleVec.empty()) {
            bool colorsInFixpoint = false;
            Colored::interval_vector_t newIntervalTuple;
            if (cfp._constraints.get_contained_colors() == _sort->size()) {
                colorsInFixpoint = true;
                for (const auto &interval : cfp._constraints) {
                    newIntervalTuple.add_interval(interval);
                }
            }
            arcIntervals._intervalTupleVec.push_back(newIntervalTuple);
            return colorsInFixpoint;
        } else {
            std::vector<Colored::interval_vector_t> newIntervalTupleVec;
            for (uint32_t i = 0; i < arcIntervals._intervalTupleVec.size(); i++) {
                auto &intervalTuple = arcIntervals._intervalTupleVec[i];
                if (intervalTuple.get_contained_colors() == _sort->size()) {
                    newIntervalTupleVec.push_back(intervalTuple);
                }
            }
            arcIntervals._intervalTupleVec = std::move(newIntervalTupleVec);
            return !arcIntervals._intervalTupleVec.empty();
        }
    }

    size_t size() const { return _sort->size(); }

    std::string to_string() const override { return _sort->get_name() + ".all"; }

    AllExpression(const ColorType *sort) : _sort(sort) { assert(sort != nullptr); }
};

typedef std::shared_ptr<AllExpression> AllExpression_ptr;

class NumberOfExpression : public ArcExpression {
  private:
    uint32_t _number;
    std::vector<ColorExpression_ptr> _color;
    AllExpression_ptr _all;

  public:
    Multiset eval(const ExpressionContext &context) const override {
        std::vector<std::pair<const Color *, uint32_t>> col;
        if (!_color.empty()) {
            for (const auto &elem : _color) {
                col.push_back(std::make_pair(elem->eval(context), _number));
            }
        } else if (_all != nullptr) {
            col = _all->eval(context);
            for (auto &pair : col) {
                pair.second = pair.second * _number;
            }
        }

        return Multiset(col);
    }
    bool is_eligible_for_symmetry(std::vector<uint32_t> &numbers) const override {
        // Not entirely sure what to do if there is more than one colorExpression, but should
        // probably return false
        if (_color.size() > 1) {
            return false;
        }
        numbers.emplace_back(_number);
        // Maybe we need to check color expression also
        return true;
    }

    bool is_tuple() const override {
        for (const auto &colorExpr : _color) {
            if (colorExpr->is_tuple()) {
                return true;
            }
        }
        return false;
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        if (_all != nullptr)
            return;
        for (const auto &elem : _color) {
            // TODO: can there be more than one element in a number of expression?
            elem->get_variables(variables, varPositions, varModifierMap, includeSubtracts, index);
        }
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        if (_all != nullptr) {
            return _all->get_arc_intervals(arcIntervals, cfp, index, modifier);
        }
        for (const auto &elem : _color) {
            if (!elem->get_arc_intervals(arcIntervals, cfp, index, modifier)) {
                return false;
            }
        }
        return true;
    }

    std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        std::vector<Colored::interval_vector_t> intervalsVec;
        if (_all == nullptr) {
            for (const auto &elem : _color) {
                for (const auto &varMap : varMapVec) {
                    intervalsVec.push_back(elem->get_output_intervals(varMap, colortypes));
                }
            }
        } else {
            intervalsVec.push_back(_all->get_output_intervals(varMapVec, colortypes));
        }
        return intervalsVec;
    }

    void get_constants(PositionColorsMap &constantMap, uint32_t &index) const override {
        if (_all != nullptr)
            _all->get_constants(constantMap, index);
        else
            for (const auto &elem : _color) {
                std::unordered_map<uint32_t, const Color *> elemMap;
                elem->get_constants(elemMap, index);
                for (const auto &pair : elemMap) {
                    constantMap[pair.first].push_back(pair.second);
                }
            }
    }

    uint32_t weight() const override {
        if (_all == nullptr)
            return _number * _color.size();
        else
            return _number * _all->size();
    }

    bool is_all() const { return (bool)_all; }

    bool is_single_color() const { return !is_all() && _color.size() == 1; }

    uint32_t number() const { return _number; }

    std::string to_string() const override {
        if (is_all())
            return std::to_string(_number) + "'(" + _all->to_string() + ")";
        std::string res = std::to_string(_number) + "'(" + _color[0]->to_string() + ")";
        for (uint32_t i = 1; i < _color.size(); ++i) {
            res += " + ";
            res += std::to_string(_number) + "'(" + _color[i]->to_string() + ")";
        }
        return res;
    }

    NumberOfExpression(std::vector<ColorExpression_ptr> &&color, uint32_t number = 1)
        : _number(number), _color(std::move(color)), _all(nullptr) {}
    NumberOfExpression(AllExpression_ptr &&all, uint32_t number = 1)
        : _number(number), _color(), _all(std::move(all)) {}
};

typedef std::shared_ptr<NumberOfExpression> NumberOfExpression_ptr;

class AddExpression : public ArcExpression {
  private:
    std::vector<ArcExpression_ptr> _constituents;

  public:
    Multiset eval(const ExpressionContext &context) const override {
        Multiset ms;
        for (const auto &expr : _constituents) {
            ms += expr->eval(context);
        }
        return ms;
    }

    bool is_eligible_for_symmetry(std::vector<uint32_t> &numbers) const override {
        for (const auto &elem : _constituents) {
            if (!elem->is_eligible_for_symmetry(numbers)) {
                return false;
            }
        }

        if (numbers.size() < 2) {
            return false;
        }
        // pick a number
        // every number has to be equal
        uint32_t firstNumber = numbers[0];
        for (uint32_t number : numbers) {
            if (firstNumber != number) {
                return false;
            }
        }
        return true;
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        for (const auto &elem : _constituents) {
            for (auto &pair : varModifierMap) {
                std::unordered_map<uint32_t, int32_t> newMap;
                pair.second.push_back(newMap);
            }
            elem->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        }
    }

    bool is_tuple() const override {
        for (const auto &arcExpr : _constituents) {
            if (arcExpr->is_tuple()) {
                return true;
            }
        }
        return false;
    }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        for (const auto &elem : _constituents) {
            uint32_t newIndex = 0;
            Colored::ArcIntervals newArcIntervals;
            std::vector<uint32_t> intervalsForRemoval;
            std::vector<Colored::interval_t> newIntervals;
            if (!elem->get_arc_intervals(newArcIntervals, cfp, newIndex, modifier)) {
                return false;
            }

            if (newArcIntervals._intervalTupleVec.empty()) {
                return false;
            }

            arcIntervals._intervalTupleVec.insert(arcIntervals._intervalTupleVec.end(),
                                                  newArcIntervals._intervalTupleVec.begin(),
                                                  newArcIntervals._intervalTupleVec.end());
        }
        return true;
    }

    std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        std::vector<Colored::interval_vector_t> intervalsVec;

        for (const auto &elem : _constituents) {
            auto nestedIntervals = elem->get_output_intervals(varMapVec, colortypes);

            intervalsVec.insert(intervalsVec.end(), nestedIntervals.begin(), nestedIntervals.end());
        }
        return intervalsVec;
    }

    void get_constants(PositionColorsMap &constantMap, uint32_t &index) const override {
        uint32_t indexCopy = index;
        for (const auto &elem : _constituents) {
            uint32_t localIndex = indexCopy;
            elem->get_constants(constantMap, localIndex);
        }
    }

    uint32_t weight() const override {
        uint32_t res = 0;
        for (const auto &expr : _constituents) {
            res += expr->weight();
        }
        return res;
    }

    std::string to_string() const override {
        std::string res = _constituents[0]->to_string();
        for (uint32_t i = 1; i < _constituents.size(); ++i) {
            res += " + " + _constituents[i]->to_string();
        }
        return res;
    }

    AddExpression(std::vector<ArcExpression_ptr> &&constituents)
        : _constituents(std::move(constituents)) {}
};

class SubtractExpression : public ArcExpression {
  private:
    ArcExpression_ptr _left;
    ArcExpression_ptr _right;

  public:
    Multiset eval(const ExpressionContext &context) const override {
        return _left->eval(context) - _right->eval(context);
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _left->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        // We ignore the restrictions imposed by the subtraction for now
        if (includeSubtracts) {
            _right->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
        }
    }

    bool is_tuple() const override { return _left->is_tuple() || _right->is_tuple(); }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        return _left->get_arc_intervals(arcIntervals, cfp, index, modifier);
        // We ignore the restrictions imposed by the subtraction for now
        //_right->get_arc_intervals(arcIntervals, cfp, &rightIndex);
    }

    std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        // We could maybe reduce the intervals slightly by checking if the upper or lower bound is
        // being subtracted
        return _left->get_output_intervals(varMapVec, colortypes);
    }

    void get_constants(PositionColorsMap &constantMap, uint32_t &index) const override {
        uint32_t rIndex = index;
        _left->get_constants(constantMap, index);
        _right->get_constants(constantMap, rIndex);
    }

    uint32_t weight() const override {
        auto *left = dynamic_cast<NumberOfExpression *>(_left.get());
        if (!left || !left->is_all()) {
            throw WeightException("Left constituent of subtract is not an all expression!");
        }
        auto *right = dynamic_cast<NumberOfExpression *>(_right.get());
        if (!right || !right->is_single_color()) {
            throw WeightException(
                "Right constituent of subtract is not a single color number of expression!");
        }

        uint32_t val = std::min(left->number(), right->number());
        return _left->weight() - val;
    }

    std::string to_string() const override {
        return _left->to_string() + " - " + _right->to_string();
    }

    SubtractExpression(ArcExpression_ptr &&left, ArcExpression_ptr &&right)
        : _left(std::move(left)), _right(std::move(right)) {}
};

class ScalarProductExpression : public ArcExpression {
  private:
    uint32_t _scalar;
    ArcExpression_ptr _expr;

  public:
    Multiset eval(const ExpressionContext &context) const override {
        return _expr->eval(context) * _scalar;
    }

    void get_variables(std::set<const Colored::Variable *> &variables,
                       PositionVariableMap &varPositions, VariableModifierMap &varModifierMap,
                       bool includeSubtracts, uint32_t &index) const override {
        _expr->get_variables(variables, varPositions, varModifierMap, includeSubtracts);
    }

    bool is_tuple() const override { return _expr->is_tuple(); }

    bool get_arc_intervals(Colored::ArcIntervals &arcIntervals,
                           const PetriEngine::Colored::ColorFixpoint &cfp, uint32_t &index,
                           int32_t modifier) const override {
        return _expr->get_arc_intervals(arcIntervals, cfp, index, modifier);
    }

    std::vector<Colored::interval_vector_t>
    get_output_intervals(const std::vector<VariableIntervalMap> &varMapVec,
                         std::vector<const Colored::ColorType *> &colortypes) const override {
        return _expr->get_output_intervals(varMapVec, colortypes);
    }

    void get_constants(PositionColorsMap &constantMap, uint32_t &index) const override {
        _expr->get_constants(constantMap, index);
    }

    uint32_t weight() const override { return _scalar * _expr->weight(); }

    std::string to_string() const override {
        return std::to_string(_scalar) + " * " + _expr->to_string();
    }

    ScalarProductExpression(ArcExpression_ptr &&expr, uint32_t scalar)
        : _scalar(std::move(scalar)), _expr(expr) {}
};
} // namespace Colored
} // namespace PetriEngine

#endif /* COLORED_EXPRESSIONS_H */
