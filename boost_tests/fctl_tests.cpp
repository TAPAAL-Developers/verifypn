/* Copyright (C) 2023  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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

#define BOOST_TEST_MODULE fctl

#include "FCTL/DependencyGraph/ManualFDG.h"
#include "FCTL/Algorithm/FCertainZeroFPA.h"
#include <boost/test/unit_test.hpp>

using namespace Featured;
using namespace DependencyGraph;
using namespace Algorithm;
namespace utf = boost::unit_test;


bool test_all(BasicDependencyGraph &dg, bool expected)
{
    for (auto search : {Strategy::DFS, Strategy::BFS}) {
        FCertainZeroFPA alg{search};
        if (alg.search(dg) != expected) {
            std::cerr << "Failure in " << static_cast<int>(search) << ".\n";
            return false;
        }
    }
    return true;
}

BOOST_AUTO_TEST_CASE(TestTest) {
    DependencyGraph::ManualFDG<int> dg;
    dg.add_hyperedge(0, {1});
    dg.add_hyperedge(1, {2});
    dg.add_hyperedge(2, {0});
    dg.add_hyperedge(2, {0, 3});
    dg.add_hyperedge(2, {3});
    dg.add_hyperedge(3, {});
    dg.add_hyperedge(3, {2});

    FCertainZeroFPA alg{Strategy::DFS};
    BOOST_REQUIRE(alg.search(dg) == true);
    //test_all(dg, true);
}


BOOST_AUTO_TEST_CASE(ParseDgTest) {
    std::stringstream ss{
            std::string{"0 1\n"} +
            "1 2\n" +
            "2 0\n" +
            "2 0 3\n" +
            "2 3\n" +
            "3 \n" +
            "3 2"};
    auto dg = DependencyGraph::parse_dg<int>(ss);

    FCertainZeroFPA alg{Strategy::DFS};
    BOOST_REQUIRE(dg.initialConfiguration() == dg.get_config(0));

    auto c0 = dg.get_config(0);
    auto c1 = dg.get_config(1);
    auto c2 = dg.get_config(2);
    auto c3 = dg.get_config(3);

    auto e0 = dg.successors(c0);
    BOOST_REQUIRE_EQUAL(e0.size(), 1);
    BOOST_REQUIRE_EQUAL(e0[0]->targets.front().conf, c1);

    auto e1 = dg.successors(dg.get_config(1));
    BOOST_REQUIRE_EQUAL(e1.size(), 1);
    BOOST_REQUIRE_EQUAL(e1[0]->targets.front().conf, c2);

    auto e2 = dg.successors(dg.get_config(2));
    BOOST_REQUIRE_EQUAL(e2.size(), 3);
    BOOST_REQUIRE_EQUAL(e2[0]->targets.front().conf, c0);

    BOOST_REQUIRE(std::find_if(std::begin(e2[1]->targets), std::end(e2[1]->targets),
                               [&](auto &suc) { return suc.conf == c0; }) != std::end(e2[1]->targets));
    BOOST_REQUIRE(std::find_if(std::begin(e2[1]->targets), std::end(e2[1]->targets),
                               [&](auto &suc) { return suc.conf == c3; }) != std::end(e2[1]->targets));

    BOOST_REQUIRE_EQUAL(e2[2]->targets.front().conf, c3);

    auto e3 = dg.successors(dg.get_config(3));
    BOOST_REQUIRE_EQUAL(e3.size(), 2);
    BOOST_REQUIRE(e3[0]->targets.empty());

    BOOST_REQUIRE_EQUAL(e3[1]->targets.front().conf, c2);

    test_all(dg, true);
}

BOOST_AUTO_TEST_CASE(ExamplesFromPapers) {

    ManualFDG<int> dg;
    dg.add_hyperedge(1, {2}, "p");
    dg.add_hyperedge(1, {3}, "w & !p");
    dg.add_hyperedge(2, {1}, "p");
    dg.add_hyperedge(3, {1}, "w & !p");
    dg.add_hyperedge(2, {3}, "p");
    dg.add_hyperedge(3, {2}, "p");
    dg.add_hyperedge(3, {4}, "l");
    dg.add_hyperedge(3, {4}, "s");
    dg.add_hyperedge(4, {3}, "s");
    dg.add_hyperedge(3, {5}, "h");
    dg.add_hyperedge(5, {3}, "h");
    dg.add_hyperedge(5, {4}, "h");
    dg.add_hyperedge(5, {6});

    dg.set_assignment(6, bddtrue, bddfalse);

    test_all(dg, false);
}