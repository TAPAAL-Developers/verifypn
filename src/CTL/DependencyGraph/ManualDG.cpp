/* Copyright (C) 2022  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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


#include "CTL/DependencyGraph/ManualDG.h"
#include <sstream>

namespace DependencyGraph {
    static void parse_assign_(const std::string& s, ManualDG<std::string>& graph) {
        std::istringstream is{s};
        std::string id;
        int assignment;
        is >> id;
        is >> assignment;
        if (!is) {
            throw base_error{"Malformed assignment statement " + s};
        }
        graph.set_assignment(id, assignment);
    }

    static void parse_negation_(const std::string& s, ManualDG<std::string>& graph) {
        std::istringstream is{s};
        std::string source, target;
        int assignment;
        is >> source >> target;
        if (!is) {
            throw base_error{"Malformed negation edge specification " + s};
        }
        graph.add_negation(source, target);
    }

    static void parse_hyperedge_(const std::string& s, ManualDG<std::string>& graph) {
        std::istringstream is{s};
        std::string id;
        is >> id;
        std::string suc;
        std::vector<std::string> sucs;
        while (is >> suc) {
            sucs.push_back(suc);
        }
        graph.add_hyperedge(id, sucs);
    }


    /**
     * Construct a fixed dependency graph from a structured text format.
     * The format should be a list of lines of form either
     *   - c x1 x2 ... xn
     *     says to add hyperedge (c, {x1, x2, ..., xn})
     *   - # c x
     *     says the assignment of c is x (x should be one of {0, 1})
     *   - ! c v
     *     adds a negation edge c, v.
     *     WARNING: No validation is done to check that negation edges do not occur in cycles.
     *     Do not expect sane results if you make a negation cycle.
     *   where c, x, xi are arbitrary strings (except #).
     *   The first mentioned configuration name is taken to be the root node.
     * @param is
     * @return
     */
    ManualDG<std::string> parse_dg(std::istream& is) {
        ManualDG<std::string> dg;

        std::string line;
        while (is) {
            is >> std::ws;
            if (!std::getline(is, line)) break;

            switch (line.at(0)) {
                case '#':
                    parse_assign_(line, dg);
                    break;
                case '!':
                    parse_negation_(line, dg);
                    break;
                default:
                    parse_hyperedge_(line, dg);
                    break;
            }
        }
        return dg;
    }

    ManualDG<std::string> parse_dg(const std::string& s) {
        std::stringstream ss{s};
        return parse_dg(ss);
    }

    ManualDG<std::string> parse_dg(const char* s) {
        std::stringstream ss{s};
        return parse_dg(ss);
    }
}