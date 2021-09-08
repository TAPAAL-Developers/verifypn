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
#ifndef PNMLPARSER_H
#define PNMLPARSER_H

#include <fstream>
#include <map>
#include <rapidxml.hpp>
#include <string>
#include <vector>

#include "../PetriEngine/AbstractPetriNetBuilder.h"
#include "../PetriEngine/Colored/ColoredNetStructures.h"
#include "../PetriEngine/Colored/Colors.h"
#include "../PetriEngine/Colored/EquivalenceClass.h"
#include "../PetriEngine/Colored/Expressions.h"
#include "../PetriEngine/PQL/PQL.h"

class PNMLParser {

    struct Arc {
        std::string _source, _target;
        int _weight;
        bool _inhib;
        PetriEngine::Colored::ArcExpression_ptr _expr;
    };
    using ArcList = std::vector<Arc>;
    using ArcIter = ArcList::iterator;

    struct Transition {
        std::string _id;
        double _x, _y;
        PetriEngine::Colored::GuardExpression_ptr _expr;
    };
    using TransitionList = std::vector<Transition>;
    using TransitionIter = TransitionList::iterator;

    struct NodeName {
        std::string _id;
        bool _is_place;
    };
    using NodeNameMap = std::unordered_map<std::string, NodeName>;

    using ColorTypeMap = std::unordered_map<std::string, const PetriEngine::Colored::ColorType *>;
    using VariableMap = std::unordered_map<std::string, const PetriEngine::Colored::Variable *>;

  public:
    struct Query {
        std::string _name, _text;
    };

    PNMLParser() { _builder = nullptr; }
    void parse(std::ifstream &xml, PetriEngine::AbstractPetriNetBuilder *builder);

    auto get_queries() -> std::vector<Query> { return _queries; }

  private:
    void parse_element(rapidxml::xml_node<> *element);
    void parse_place(rapidxml::xml_node<> *element);
    void parse_arc(rapidxml::xml_node<> *element, bool inhibitor = false);
    void parse_transition(rapidxml::xml_node<> *element);
    void parse_declarations(rapidxml::xml_node<> *element);
    void parse_partitions(rapidxml::xml_node<> *element);
    void parse_named_sort(rapidxml::xml_node<> *element);
    auto parse_arc_expression(rapidxml::xml_node<> *element)
        -> PetriEngine::Colored::ArcExpression_ptr;
    auto parse_guard_expression(rapidxml::xml_node<> *element, bool notFlag)
        -> PetriEngine::Colored::GuardExpression_ptr;
    auto parse_color_expression(rapidxml::xml_node<> *element)
        -> PetriEngine::Colored::ColorExpression_ptr;
    auto parse_all_expression(rapidxml::xml_node<> *element)
        -> PetriEngine::Colored::AllExpression_ptr;
    auto parse_user_sort(rapidxml::xml_node<> *element) -> const PetriEngine::Colored::ColorType *;
    auto parse_number_of_expression(rapidxml::xml_node<> *element)
        -> PetriEngine::Colored::ArcExpression_ptr;
    void collect_colors_in_tuple(
        rapidxml::xml_node<> *element,
        std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> &collectedColors);
    auto construct_add_expression_from_tuple_expression(
        rapidxml::xml_node<> *element,
        std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> collectedColors,
        uint32_t numberof) -> PetriEngine::Colored::ArcExpression_ptr;
    void parse_transport_arc(rapidxml::xml_node<> *element);
    void parse_value(rapidxml::xml_node<> *element, std::string &text);
    auto parse_number_constant(rapidxml::xml_node<> *element) -> uint32_t;
    void parse_position(rapidxml::xml_node<> *element, double &x, double &y);
    void parse_queries(rapidxml::xml_node<> *element);
    auto find_color(const char *name) const -> const PetriEngine::Colored::Color *;
    auto find_color_for_int_range(const char *value, uint32_t start, uint32_t end) const
        -> const PetriEngine::Colored::Color *;
    auto find_partition_colors(rapidxml::xml_node<> *element) const
        -> std::vector<PetriEngine::Colored::ColorExpression_ptr>;
    auto
    cartesian_product(const std::vector<PetriEngine::Colored::ColorExpression_ptr> &rightVector,
                      const std::vector<PetriEngine::Colored::ColorExpression_ptr> &leftVector)
        -> std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>>;
    auto cartesian_product(
        const std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>> &rightVector,
        const std::vector<PetriEngine::Colored::ColorExpression_ptr> &leftVector)
        -> std::vector<std::vector<PetriEngine::Colored::ColorExpression_ptr>>;
    PetriEngine::AbstractPetriNetBuilder *_builder;
    NodeNameMap _id2name;
    ArcList _arcs;
    TransitionList _transitions;
    ColorTypeMap _colorTypes;
    VariableMap _variables;
    bool _isColored;
    std::vector<Query> _queries;
    std::vector<PetriEngine::Colored::color_type_partition_t> _partitions;
    std::vector<std::pair<char *, PetriEngine::Colored::ProductType *>> _missingCTs;
};

#endif // PNMLPARSER_H