/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <string>

#include "PetriParse/PNMLParser.h"
#include "errorcodes.h"

using namespace PetriEngine;
using namespace PetriEngine::PQL;

void PNMLParser::parse(std::ifstream &xml, AbstractPetriNetBuilder *builder) {
    // Clear any left overs
    _id2name.clear();
    _arcs.clear();
    _transitions.clear();
    _colorTypes.clear();

    // Set the builder
    this->_builder = builder;

    // Parser the xml
    rapidxml::xml_document<> doc;
    std::vector<char> buffer((std::istreambuf_iterator<char>(xml)),
                             std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    doc.parse<0>(&buffer[0]);

    rapidxml::xml_node<> *root = doc.first_node();
    if (strcmp(root->name(), "pnml") != 0) {
        throw base_error_t("expecting <pnml> tag as root-node in xml tree.");
    }

    auto declarations = root->first_node("declaration");
    if (declarations == nullptr) {
        declarations = root->first_node("net")->first_node("declaration");
    }

    _isColored = declarations != nullptr;
    if (_isColored) {
        builder->enable_colors();
        parse_declarations(declarations);
    }

    parse_element(root);

    // Add all the transition
    for (auto &transition : _transitions)
        if (!_isColored) {
            builder->add_transition(transition._id, transition._x, transition._y);
        } else {
            builder->add_transition(transition._id, transition._expr, transition._x, transition._y);
        }

    // Add all the arcs
    for (auto &arc : _arcs) {
        auto a = arc;

        // Check that source id exists
        if (_id2name.find(arc._source) == _id2name.end()) {
            fprintf(stderr, "XML Parsing error: Arc source with id=\"%s\" wasn't found!\n",
                    arc._source.c_str());
            continue;
        }
        // Check that target id exists
        if (_id2name.find(arc._target) == _id2name.end()) {
            fprintf(stderr, "XML Parsing error: Arc target with id=\"%s\" wasn't found!\n",
                    arc._target.c_str());
            continue;
        }
        // Find source and target
        node_name_t source = _id2name[arc._source];
        node_name_t target = _id2name[arc._target];

        if (source._is_place && !target._is_place) {
            if (!_isColored) {
                builder->add_input_arc(source._id, target._id, arc._inhib, arc._weight);
            } else {
                builder->add_input_arc(source._id, target._id, arc._expr, arc._inhib, arc._weight);
            }

        } else if (!source._is_place && target._is_place) {
            if (!_isColored) {
                builder->add_output_arc(source._id, target._id, arc._weight);
            } else {
                builder->add_output_arc(source._id, target._id, arc._expr);
            }
        } else {
            fprintf(stderr,
                    "XML Parsing error: Arc from \"%s\" to \"%s\" is neither input nor output!\n",
                    source._id.c_str(), target._id.c_str());
        }
    }

    // Unset the builder
    this->_builder = nullptr;

    // Cleanup
    _id2name.clear();
    _arcs.clear();
    _transitions.clear();
    _colorTypes.clear();
    builder->sort();
}

void PNMLParser::parse_declarations(rapidxml::xml_node<> *element) {
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "namedsort") == 0) {
            parse_named_sort(it);
        } else if (strcmp(it->name(), "variabledecl") == 0) {
            auto var = new PetriEngine::Colored::Variable{it->first_attribute("id")->value(),
                                                          parse_user_sort(it)};
            _variables[it->first_attribute("id")->value()] = var;
        } else if (strcmp(it->name(), "partition") == 0) {
            parse_partitions(it);
        } else {
            parse_declarations(it);
        }
    }

    for (auto missingCTPair : _missingCTs) {
        if (_colorTypes.count(missingCTPair.first) == 0) {
            throw base_error_t("Unable to find colortype ", missingCTPair.first,
                               " used in product type ", missingCTPair.second->get_name());
        }
        missingCTPair.second->add_type(_colorTypes[missingCTPair.first]);
    }
    _missingCTs.clear();
}

void PNMLParser::parse_partitions(rapidxml::xml_node<> *element) {
    auto partitionCT = parse_user_sort(element);
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "partitionelement") == 0) {
            auto id = it->first_attribute("id")->value();
            std::vector<const PetriEngine::Colored::Color *> colors;
            for (auto partitionElement = it->first_node(); partitionElement;
                 partitionElement = partitionElement->next_sibling()) {
                colors.push_back(partitionCT->operator[](
                    partitionElement->first_attribute("declaration")->value()));
            }
            _partitions.push_back({colors, id});
        }
    }
}

void PNMLParser::parse_named_sort(rapidxml::xml_node<> *element) {
    auto type = element->first_node();
    const PetriEngine::Colored::ColorType *fct = nullptr;
    if (strcmp(type->name(), "dot") == 0) {
        fct = Colored::ColorType::dot_instance();
    } else {
        if (strcmp(type->name(), "productsort") == 0) {
            auto ct = new PetriEngine::Colored::ProductType(
                std::string(element->first_attribute("id")->value()));
            bool missingType = false;
            for (auto it = type->first_node(); it; it = it->next_sibling()) {
                if (strcmp(it->name(), "usersort") == 0) {
                    auto ctName = it->first_attribute("declaration")->value();
                    if (!missingType && _colorTypes.count(ctName)) {
                        ct->add_type(_colorTypes[ctName]);
                    } else {
                        missingType = true;
                        _missingCTs.emplace_back(ctName, ct);
                    }
                }
            }
            fct = ct;
        } else {
            auto ct = new PetriEngine::Colored::ColorType(
                std::string(element->first_attribute("id")->value()));
            if (strcmp(type->name(), "finiteintrange") == 0) {

                uint32_t start = (uint32_t)atoll(type->first_attribute("start")->value());
                uint32_t end = (uint32_t)atoll(type->first_attribute("end")->value());

                for (uint32_t i = start; i <= end; i++) {
                    ct->add_color(std::to_string(i).c_str());
                }
                fct = ct;
            } else {
                for (auto it = type->first_node(); it; it = it->next_sibling()) {
                    auto id = it->first_attribute("id");
                    assert(id != nullptr);
                    ct->add_color(id->value());
                }
            }
            fct = ct;
        }
    }

    std::string id = element->first_attribute("id")->value();
    _colorTypes[id] = fct;
    _builder->add_color_type(id, fct);
}

auto PNMLParser::parse_arc_expression(rapidxml::xml_node<> *element)
    -> PetriEngine::Colored::ArcExpression_ptr {
    if (strcmp(element->name(), "numberof") == 0) {
        return parse_number_of_expression(element);
    } else if (strcmp(element->name(), "add") == 0) {
        std::vector<PetriEngine::Colored::ArcExpression_ptr> constituents;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            constituents.push_back(parse_arc_expression(it));
        }
        return std::make_shared<PetriEngine::Colored::AddExpression>(std::move(constituents));
    } else if (strcmp(element->name(), "subtract") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        auto res = std::make_shared<PetriEngine::Colored::SubtractExpression>(
            parse_arc_expression(left), parse_arc_expression(right));
        auto next = right;
        while ((next = next->next_sibling())) {
            res = std::make_shared<PetriEngine::Colored::SubtractExpression>(
                res, parse_arc_expression(next));
        }
        return res;
    } else if (strcmp(element->name(), "scalarproduct") == 0) {
        auto scalar = element->first_node();
        auto ms = scalar->next_sibling();
        return std::make_shared<PetriEngine::Colored::ScalarProductExpression>(
            parse_arc_expression(ms), parse_number_constant(scalar));
    } else if (strcmp(element->name(), "all") == 0) {
        return parse_number_of_expression(element->parent());
    } else if (strcmp(element->name(), "subterm") == 0 ||
               strcmp(element->name(), "structure") == 0) {
        return parse_arc_expression(element->first_node());
    } else if (strcmp(element->name(), "tuple") == 0) {
        std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> collectedColors;
        collect_colors_in_tuple(element, collectedColors);
        auto expr = construct_add_expression_from_tuple_expression(element, collectedColors, 1);
        return expr;
    }
    printf("Could not parse '%s' as an arc expression\n", element->name());
    assert(false);
    return nullptr;
}

auto PNMLParser::construct_add_expression_from_tuple_expression(
    rapidxml::xml_node<> *element,
    std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> collectedColors,
    uint32_t numberof) -> PetriEngine::Colored::ArcExpression_ptr {
    std::vector<PetriEngine::Colored::ArcExpression_ptr> numberOfExpressions;
    if (collectedColors.size() < 2) {
        for (const auto &exp : collectedColors[0]) {
            std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
            colors.push_back(exp);
            numberOfExpressions.push_back(
                std::make_shared<PetriEngine::Colored::NumberOfExpression>(std::move(colors),
                                                                           numberof));
        }
    } else {
        auto initCartesianSet = cartesian_product(collectedColors[0], collectedColors[1]);
        for (uint32_t i = 2; i < collectedColors.size(); i++) {
            initCartesianSet = cartesian_product(initCartesianSet, collectedColors[i]);
        }
        for (const auto &set : initCartesianSet) {
            std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
            colors.reserve(set.size());
            for (const auto &color : set) {
                colors.push_back(color);
            }
            std::shared_ptr<PetriEngine::Colored::TupleExpression> tupleExpr =
                std::make_shared<PetriEngine::Colored::TupleExpression>(std::move(colors));
            tupleExpr->set_color_type(tupleExpr->get_color_type(_colorTypes));
            std::vector<PetriEngine::Colored::ColorExpression_ptr> placeholderVector;
            placeholderVector.push_back(tupleExpr);
            numberOfExpressions.push_back(
                std::make_shared<PetriEngine::Colored::NumberOfExpression>(
                    std::move(placeholderVector), numberof));
        }
    }
    return std::make_shared<PetriEngine::Colored::AddExpression>(std::move(numberOfExpressions));
}

auto PNMLParser::cartesian_product(
    const std::vector<PetriEngine::Colored::ColorExpression_ptr> &rightSet,
    const std::vector<PetriEngine::Colored::ColorExpression_ptr> &leftSet)
    -> std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> {
    std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> returnSet;
    for (const auto &expr : rightSet) {
        for (const auto &expr2 : leftSet) {
            std::vector<PetriEngine::Colored::ColorExpression_ptr> toAdd;
            toAdd.push_back(expr);
            toAdd.push_back(expr2);
            returnSet.push_back(toAdd);
        }
    }
    return returnSet;
}
auto PNMLParser::cartesian_product(
    const std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> &rightSet,
    const std::vector<PetriEngine::Colored::ColorExpression_ptr> &leftSet)
    -> std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> {
    std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> returnSet;
    for (const auto &set : rightSet) {
        for (const auto &expr2 : leftSet) {
            auto setCopy = set;
            setCopy.push_back(expr2);
            returnSet.push_back(std::move(setCopy));
        }
    }
    return returnSet;
}

void PNMLParser::collect_colors_in_tuple(
    rapidxml::xml_node<> *element,
    std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> &collectedColors) {
    if (strcmp(element->name(), "tuple") == 0) {
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            collect_colors_in_tuple(it->first_node(), collectedColors);
        }
    } else if (strcmp(element->name(), "all") == 0) {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> expressionsToAdd;
        auto expr = parse_all_expression(element);
        std::unordered_map<uint32_t, std::vector<const PetriEngine::Colored::Color *>> constantMap;
        uint32_t index = 0;
        expr->get_constants(constantMap, index);
        for (const auto &positionColors : constantMap) {
            for (auto color : positionColors.second) {
                expressionsToAdd.push_back(
                    std::make_shared<PetriEngine::Colored::UserOperatorExpression>(color));
            }
        }
        collectedColors.push_back(expressionsToAdd);
    } else if (strcmp(element->name(), "add") == 0) {
        std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> intermediateColors;
        std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> intermediateColors2;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            collect_colors_in_tuple(it, intermediateColors2);
            if (intermediateColors.empty()) {
                intermediateColors = intermediateColors2;
            } else {
                for (uint32_t i = 0; i < intermediateColors.size(); i++) {
                    intermediateColors[i].insert(intermediateColors[i].end(),
                                                 intermediateColors2[i].begin(),
                                                 intermediateColors2[i].end());
                }
            }
        }
        for (auto &colorVec : intermediateColors) {
            collectedColors.push_back(std::move(colorVec));
        }
    } else if (strcmp(element->name(), "subterm") == 0 ||
               strcmp(element->name(), "structure") == 0) {
        collect_colors_in_tuple(element->first_node(), collectedColors);
    } else if (strcmp(element->name(), "finiteintrangeconstant") == 0) {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> expressionsToAdd;
        auto value = element->first_attribute("value")->value();
        auto intRangeElement = element->first_node("finiteintrange");
        uint32_t start = (uint32_t)atoll(intRangeElement->first_attribute("start")->value());
        uint32_t end = (uint32_t)atoll(intRangeElement->first_attribute("end")->value());
        expressionsToAdd.push_back(std::make_shared<PetriEngine::Colored::UserOperatorExpression>(
            find_color_for_int_range(value, start, end)));
        collectedColors.push_back(expressionsToAdd);
    } else if (strcmp(element->name(), "useroperator") == 0 ||
               strcmp(element->name(), "dotconstant") == 0 ||
               strcmp(element->name(), "variable") == 0 ||
               strcmp(element->name(), "successor") == 0 ||
               strcmp(element->name(), "predecessor") == 0) {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> expressionsToAdd =
            find_partition_colors(element);
        if (expressionsToAdd.empty()) {
            auto color = parse_color_expression(element);
            expressionsToAdd.push_back(color);
        }
        collectedColors.push_back(expressionsToAdd);
    } else {
        printf("Could not parse '%s' as an arc expression when collecting tuple colors\n",
               element->name());
    }
}

auto PNMLParser::parse_guard_expression(rapidxml::xml_node<> *element, bool notFlag)
    -> PetriEngine::Colored::GuardExpression_ptr {
    if (strcmp(element->name(), "lt") == 0 || strcmp(element->name(), "lessthan") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::GreaterThanEqExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::LessThanExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "gt") == 0 || strcmp(element->name(), "greaterthan") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();

        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::LessThanEqExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::GreaterThanExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "leq") == 0 ||
               strcmp(element->name(), "lessthanorequal") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();

        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::GreaterThanExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::LessThanEqExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "geq") == 0 ||
               strcmp(element->name(), "greaterthanorequal") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();

        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::LessThanExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::GreaterThanEqExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "eq") == 0 || strcmp(element->name(), "equality") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::InequalityExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::EqualityExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "neq") == 0 || strcmp(element->name(), "inequality") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();

        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::EqualityExpression>(
                parse_color_expression(left), parse_color_expression(right));
        } else {
            return std::make_shared<PetriEngine::Colored::InequalityExpression>(
                parse_color_expression(left), parse_color_expression(right));
        }
    } else if (strcmp(element->name(), "not") == 0) {
        return parse_guard_expression(element->first_node(), true);
    } else if (strcmp(element->name(), "and") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        if (notFlag) {
            return std::make_shared<PetriEngine::Colored::OrExpression>(
                parse_guard_expression(left, true), parse_guard_expression(right, true));
        } else {
            return std::make_shared<PetriEngine::Colored::AndExpression>(
                parse_guard_expression(left, false), parse_guard_expression(right, false));
        }
    } else if (strcmp(element->name(), "or") == 0) {
        auto left = element->first_node();
        auto right = left->next_sibling();
        // There must only be one constituent
        if (right == nullptr) {
            return parse_guard_expression(left, notFlag);
        }
        if (notFlag) {
            auto parentAnd = std::make_shared<PetriEngine::Colored::AndExpression>(
                parse_guard_expression(left, true), parse_guard_expression(right, true));
            for (auto it = right->next_sibling(); it; it = it->next_sibling()) {
                parentAnd = std::make_shared<PetriEngine::Colored::AndExpression>(
                    parentAnd, parse_guard_expression(it, true));
            }
            return parentAnd;
        } else {
            auto parentOr = std::make_shared<PetriEngine::Colored::OrExpression>(
                parse_guard_expression(left, false), parse_guard_expression(right, false));
            for (auto it = right->next_sibling(); it; it = it->next_sibling()) {
                parentOr = std::make_shared<PetriEngine::Colored::OrExpression>(
                    parentOr, parse_guard_expression(it, false));
            }
            return parentOr;
        }
    } else if (strcmp(element->name(), "subterm") == 0 ||
               strcmp(element->name(), "structure") == 0) {
        return parse_guard_expression(element->first_node(), notFlag);
    }

    printf("Could not parse '%s' as a guard expression\n", element->name());
    assert(false);
    return nullptr;
}

auto PNMLParser::parse_color_expression(rapidxml::xml_node<> *element)
    -> PetriEngine::Colored::ColorExpression_ptr {
    if (strcmp(element->name(), "dotconstant") == 0) {
        return std::make_shared<PetriEngine::Colored::DotConstantExpression>();
    } else if (strcmp(element->name(), "variable") == 0) {
        return std::make_shared<PetriEngine::Colored::VariableExpression>(
            _variables[element->first_attribute("refvariable")->value()]);
    } else if (strcmp(element->name(), "useroperator") == 0) {
        return std::make_shared<PetriEngine::Colored::UserOperatorExpression>(
            find_color(element->first_attribute("declaration")->value()));
    } else if (strcmp(element->name(), "successor") == 0) {
        return std::make_shared<PetriEngine::Colored::SuccessorExpression>(
            parse_color_expression(element->first_node()));
    } else if (strcmp(element->name(), "predecessor") == 0) {
        return std::make_shared<PetriEngine::Colored::PredecessorExpression>(
            parse_color_expression(element->first_node()));
    } else if (strcmp(element->name(), "finiteintrangeconstant") == 0) {
        auto value = element->first_attribute("value")->value();
        auto intRangeElement = element->first_node("finiteintrange");
        uint32_t start = (uint32_t)atoll(intRangeElement->first_attribute("start")->value());
        uint32_t end = (uint32_t)atoll(intRangeElement->first_attribute("end")->value());
        return std::make_shared<PetriEngine::Colored::UserOperatorExpression>(
            find_color_for_int_range(value, start, end));

    } else if (strcmp(element->name(), "tuple") == 0) {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            colors.push_back(parse_color_expression(it));
        }
        std::shared_ptr<PetriEngine::Colored::TupleExpression> tupleExpr =
            std::make_shared<PetriEngine::Colored::TupleExpression>(std::move(colors));
        tupleExpr->set_color_type(tupleExpr->get_color_type(_colorTypes));
        return tupleExpr;
    } else if (strcmp(element->name(), "subterm") == 0 ||
               strcmp(element->name(), "structure") == 0) {
        return parse_color_expression(element->first_node());
    }
    assert(false);
    return nullptr;
}

auto PNMLParser::parse_all_expression(rapidxml::xml_node<> *element)
    -> PetriEngine::Colored::AllExpression_ptr {
    if (strcmp(element->name(), "all") == 0) {
        return std::make_shared<PetriEngine::Colored::AllExpression>(parse_user_sort(element));
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parse_all_expression(element->first_node());
    }

    return nullptr;
}

auto PNMLParser::parse_user_sort(rapidxml::xml_node<> *element)
    -> const PetriEngine::Colored::ColorType * {
    if (element) {
        for (auto it = element->first_node(); it; it = it->next_sibling()) {
            if (strcmp(it->name(), "usersort") == 0) {
                return _colorTypes[it->first_attribute("declaration")->value()];
            } else if (strcmp(it->name(), "structure") == 0 || strcmp(it->name(), "type") == 0 ||
                       strcmp(it->name(), "subterm") == 0) {
                return parse_user_sort(it);
            }
        }
    }
    assert(false);
    return nullptr;
}

auto PNMLParser::parse_number_of_expression(rapidxml::xml_node<> *element)
    -> PetriEngine::Colored::ArcExpression_ptr {
    auto num = element->first_node();
    uint32_t number = parse_number_constant(num);
    rapidxml::xml_node<> *first;
    if (number) {
        first = num->next_sibling();
    } else {
        number = 1;
        first = num;
    }

    auto allExpr = parse_all_expression(first);
    if (allExpr) {
        return std::make_shared<PetriEngine::Colored::NumberOfExpression>(std::move(allExpr),
                                                                          number);
    } else {
        std::vector<PetriEngine::Colored::ColorExpression_ptr> colors;
        for (auto it = first; it; it = it->next_sibling()) {
            colors.push_back(parse_color_expression(it));
        }
        return std::make_shared<PetriEngine::Colored::NumberOfExpression>(std::move(colors),
                                                                          number);
    }
}

void PNMLParser::parse_element(rapidxml::xml_node<> *element) {

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "place") == 0) {
            parse_place(it);
        } else if (strcmp(it->name(), "transition") == 0) {
            parse_transition(it);
        } else if (strcmp(it->name(), "arc") == 0 || strcmp(it->name(), "inputArc") == 0 ||
                   strcmp(it->name(), "outputArc") == 0) {
            parse_arc(it);
        } else if (strcmp(it->name(), "transportArc") == 0) {
            parse_transport_arc(it);
        } else if (strcmp(it->name(), "inhibitorArc") == 0) {
            parse_arc(it, true);
        } else if (strcmp(it->name(), "variable") == 0) {
            throw base_error_t("ERROR: variable not supported");
        } else if (strcmp(it->name(), "queries") == 0) {
            parse_queries(it);
        } else if (strcmp(it->name(), "k-bound") == 0) {
            throw base_error_t("ERROR: k-bound should be given as command line option -k");
        } else if (strcmp(it->name(), "query") == 0) {
            throw base_error_t(
                "ERROR: query tag not supported, please use PQL or XML-style queries instead");
        } else {
            parse_element(it);
        }
    }
}

void PNMLParser::parse_queries(rapidxml::xml_node<> *element) {
    std::string query;

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        std::string name(element->first_attribute("name")->value());
        parse_value(element, query);
        query_t q;
        q._name = name;
        q._text = query;
        this->_queries.push_back(q);
    }
}

void PNMLParser::parse_place(rapidxml::xml_node<> *element) {
    double x = 0, y = 0;
    std::string id(element->first_attribute("id")->value());

    auto initial = element->first_attribute("initialMarking");
    long long initialMarking = 0;
    PetriEngine::Colored::Multiset hlinitialMarking;
    const PetriEngine::Colored::ColorType *type = nullptr;
    if (initial)
        initialMarking = atoll(initial->value());

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parse_position(it, x, y);
        } else if (strcmp(it->name(), "initialMarking") == 0) {
            std::string text;
            parse_value(it, text);
            initialMarking = atoll(text.c_str());
        } else if (strcmp(it->name(), "hlinitialMarking") == 0) {
            std::unordered_map<const PetriEngine::Colored::Variable *,
                               const PetriEngine::Colored::Color *>
                binding;
            PetriEngine::Colored::EquivalenceVec placePartition;
            PetriEngine::Colored::expression_context_t context{binding, _colorTypes,
                                                               placePartition};
            hlinitialMarking = parse_arc_expression(it->first_node("structure"))->eval(context);
        } else if (strcmp(it->name(), "type") == 0) {
            type = parse_user_sort(it);
        }
    }

    if (initialMarking > std::numeric_limits<int>::max()) {
        throw base_error_t("Number of tokens in ", id, " exceeded ",
                           std::numeric_limits<int>::max());
    }
    // Create place
    if (!_isColored) {
        _builder->add_place(id, initialMarking, x, y);
    } else {
        if (!type) {
            throw base_error_t("Place '", id, "' is missing color type");
        } else {
            _builder->add_place(id, type, std::move(hlinitialMarking), x, y);
        }
    }
    // Map id to name
    node_name_t nn;
    nn._id = id;
    nn._is_place = true;
    _id2name[id] = nn;
}

void PNMLParser::parse_arc(rapidxml::xml_node<> *element, bool inhibitor) {
    std::string source = element->first_attribute("source")->value(),
                target = element->first_attribute("target")->value();
    int weight = 1;
    auto type = element->first_attribute("type");
    if (type && strcmp(type->value(), "timed") == 0) {
        throw base_error_t("timed arcs are not supported");
    } else if (type && strcmp(type->value(), "inhibitor") == 0) {
        inhibitor = true;
    }

    bool first = true;
    auto weightTag = element->first_attribute("weight");
    if (weightTag != nullptr) {
        weight = atoi(weightTag->value());
        assert(weight > 0);
    } else {
        for (auto it = element->first_node("inscription"); it;
             it = it->next_sibling("inscription")) {
            std::string text;
            parse_value(it, text);
            weight = atoi(text.c_str());
            if (std::find_if(text.begin(), text.end(), [](char c) {
                    return !std::isdigit(c) && !std::isblank(c);
                }) != text.end()) {
                throw base_error_t(
                    "ERROR: Found non-integer-text in inscription-tag (weight) on arc from ",
                    source, " to ", target, " with value \"", text, "\". An integer was expected.");
            }
            assert(weight > 0);
            if (!first) {
                throw base_error_t("ERROR: Multiple inscription tags in xml of a arc from ", source,
                                   " to ", target, ".");
            }
            first = false;
        }
    }

    PetriEngine::Colored::ArcExpression_ptr expr;
    first = true;
    for (auto it = element->first_node("hlinscription"); it;
         it = it->next_sibling("hlinscription")) {
        expr = parse_arc_expression(it->first_node("structure"));
        if (!first) {
            throw base_error_t("ERROR: Multiple hlinscription tags in xml of a arc from ", source,
                               " to ", target, ".");
        }
        first = false;
    }

    if (_isColored && !inhibitor)
        assert(expr != nullptr);
    arc_t arc;
    arc._source = source;
    arc._target = target;
    arc._weight = weight;
    arc._inhib = inhibitor;
    if (!inhibitor)
        arc._expr = expr;
    assert(weight > 0);

    if (weight != 0) {
        _arcs.push_back(arc);
    } else {
        throw base_error_t("ERROR: Arc from ", source, " to ", target,
                           " has non-sensible weight 0.");
    }
}

void PNMLParser::parse_transport_arc(rapidxml::xml_node<> *element) {
    std::string source = element->first_attribute("source")->value(),
                transiton = element->first_attribute("transition")->value(),
                target = element->first_attribute("target")->value();
    int weight = 1;

    for (auto it = element->first_node("inscription"); it; it = it->next_sibling("inscription")) {
        std::string text;
        parse_value(it, text);
        weight = atoi(text.c_str());
    }

    arc_t inArc;
    inArc._source = source;
    inArc._target = transiton;
    inArc._weight = weight;
    _arcs.push_back(inArc);

    arc_t outArc;
    outArc._source = transiton;
    outArc._target = target;
    outArc._weight = weight;
    _arcs.push_back(outArc);
}

void PNMLParser::parse_transition(rapidxml::xml_node<> *element) {
    transition_t t;
    t._x = 0;
    t._y = 0;
    t._id = element->first_attribute("id")->value();
    t._expr = nullptr;

    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        // name element is ignored
        if (strcmp(it->name(), "graphics") == 0) {
            parse_position(it, t._x, t._y);
        } else if (strcmp(it->name(), "condition") == 0) {
            t._expr = parse_guard_expression(it->first_node("structure"), false);
        } else if (strcmp(it->name(), "conditions") == 0) {
            throw base_error_t("conditions not supported");
        } else if (strcmp(it->name(), "assignments") == 0) {
            throw base_error_t("assignments not supported");
        }
    }
    // Add transition to list
    _transitions.push_back(t);
    // Map id to name
    node_name_t nn;
    nn._id = t._id;
    nn._is_place = false;
    _id2name[t._id] = nn;
}

void PNMLParser::parse_value(rapidxml::xml_node<> *element, std::string &text) {
    for (auto it = element->first_node(); it; it = it->next_sibling()) {
        if (strcmp(it->name(), "value") == 0 || strcmp(it->name(), "text") == 0) {
            text = it->value();
        } else
            parse_value(it, text);
    }
}

auto PNMLParser::parse_number_constant(rapidxml::xml_node<> *element) -> uint32_t {
    if (strcmp(element->name(), "numberconstant") == 0) {
        auto value = element->first_attribute("value")->value();
        return (uint32_t)atoll(value);
    } else if (strcmp(element->name(), "subterm") == 0) {
        return parse_number_constant(element->first_node());
    }
    return 0;
}

void PNMLParser::parse_position(rapidxml::xml_node<> *element, double &x, double &y) {
    for (auto it = element->first_node(); it; it = it->first_node()) {
        if (strcmp(it->name(), "position") == 0) {
            x = atof(it->first_attribute("x")->value());
            y = atof(it->first_attribute("y")->value());
        } else {
            parse_position(it, x, y);
        }
    }
}

auto PNMLParser::find_color(const char *name) const -> const PetriEngine::Colored::Color * {
    for (const auto &elem : _colorTypes) {
        auto col = (*elem.second)[name];
        if (col)
            return col;
    }
    throw base_error_t("Could not find color: ", name, "\nCANNOT_COMPUTE\n");
}

auto PNMLParser::find_partition_colors(rapidxml::xml_node<> *element) const
    -> std::vector<PetriEngine::Colored::ColorExpression_ptr> {
    std::vector<PetriEngine::Colored::ColorExpression_ptr> colorExpressions;
    char *name;
    if (strcmp(element->name(), "useroperator") == 0) {
        name = element->first_attribute("declaration")->value();
    } else if (strcmp(element->name(), "successor") == 0) {
        auto colorExpressionVec = find_partition_colors(element->first_node());
        for (auto &colorExpression : colorExpressionVec) {
            colorExpressions.push_back(std::make_shared<PetriEngine::Colored::SuccessorExpression>(
                std::move(colorExpression)));
        }
        return colorExpressions;
    } else if (strcmp(element->name(), "predecessor") == 0) {
        auto colorExpressionVec = find_partition_colors(element->first_node());
        for (auto &colorExpression : colorExpressionVec) {
            colorExpressions.push_back(
                std::make_shared<PetriEngine::Colored::PredecessorExpression>(
                    std::move(colorExpression)));
        }
        return colorExpressions;
    } else if (strcmp(element->name(), "variable") == 0 ||
               strcmp(element->name(), "dotconstant") == 0 ||
               strcmp(element->name(), "finiteintrangeconstant") == 0) {
        return colorExpressions;
    } else if (strcmp(element->name(), "subterm") == 0) {
        return find_partition_colors(element->first_node());
    } else {
        throw base_error_t("Could not find color expression in expression: ", element->name(),
                           "\nCANNOT_COMPUTE\n");
    }

    for (const auto &partition : _partitions) {
        if (strcmp(partition._name.c_str(), name) == 0) {
            for (auto color : partition._colors) {
                colorExpressions.push_back(
                    std::make_shared<PetriEngine::Colored::UserOperatorExpression>(color));
            }
        }
    }
    return colorExpressions;
}

auto PNMLParser::find_color_for_int_range(const char *value, uint32_t start, uint32_t end) const
    -> const PetriEngine::Colored::Color * {
    for (const auto &elem : _colorTypes) {
        auto col = (*elem.second)[value];
        if (col) {
            if ((*elem.second)[0].get_id() == (start - 1) &&
                (*elem.second)[(*elem.second).size() - 1].get_id() == end - 1)
                return col;
        }
    }
    throw base_error_t("Could not find color: ", value, "\nCANNOT_COMPUTE\n");
}
