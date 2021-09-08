/* VerifyPN - TAPAAL Petri Net Engine
 * Copyright (C) 2014 Jiri Srba <srba.jiri@gmail.com>
 *                    Peter Gjøl Jensen <root@petergjoel.dk>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef QUERYXMLPARSER_H
#define QUERYXMLPARSER_H

#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string.h>
#include <string>
#include <vector>

#include <rapidxml.hpp>

#include "PNMLParser.h"
#include "QueryParser.h"
using namespace PetriEngine::PQL;

class QueryXMLParser {
  public:
    QueryXMLParser();
    ~QueryXMLParser();

    std::vector<QueryItem> queries;

    bool parse(std::ifstream &xml, const std::set<size_t> &);
    void print_queries();
    void print_queries(size_t i);

  private:
    bool parse_property_set(rapidxml::xml_node<> *element, const std::set<size_t> &);
    bool parse_property(rapidxml::xml_node<> *element);
    bool parse_tags(rapidxml::xml_node<> *element);
    Condition_ptr parse_formula(rapidxml::xml_node<> *element);
    Condition_ptr parse_boolean_formula(rapidxml::xml_node<> *element);
    Expr_ptr parse_integer_expression(rapidxml::xml_node<> *element);
    std::string parse_place(rapidxml::xml_node<> *element);
    void fatal_error(const std::string &token);
};

#endif /* QUERYXMLPARSER_H */
