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

#define BOOST_TEST_MODULE ctl
#include <boost/test/unit_test.hpp>

#include "CTL/DependencyGraph/ManualDG.h"
#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/Algorithm/CertainZeroFPA.h"

#include <string>
#include <fstream>
#include <sstream>
#include <set>
#include <vector>


using namespace DependencyGraph;
using namespace Algorithm;
namespace utf = boost::unit_test;

void test_all(BasicDependencyGraph &dg, bool expected) {
    LocalFPA local{Strategy::DFS};
    BOOST_REQUIRE(local.search(dg) == expected);
    if (auto manual = dynamic_cast<ManualDG<>*>(&dg)) {
        manual->reset_state();
    }

    CertainZeroFPA czero{Strategy::DFS};
    BOOST_REQUIRE(czero.search(dg) == expected);
}


BOOST_AUTO_TEST_CASE(DirectoryTest) {
        BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(TestTest) {
    DependencyGraph::ManualDG<int> dg;
    dg.add_hyperedge(0, {1});
    dg.add_hyperedge(1, {2});
    dg.add_hyperedge(2, {0});
    dg.add_hyperedge(2, {0, 3});
    dg.add_hyperedge(2, {3});
    dg.add_hyperedge(3, {});
    dg.add_hyperedge(3, {2});

    LocalFPA alg{Strategy::DFS};
    BOOST_REQUIRE(alg.search(dg));

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

    CertainZeroFPA alg{Strategy::DFS};
    BOOST_REQUIRE(dg.initialConfiguration() == dg.get_config(0));

    auto c0 = dg.get_config(0);
    auto c1 = dg.get_config(1);
    auto c2 = dg.get_config(2);
    auto c3 = dg.get_config(3);

    auto e0 = dg.successors(c0);
    BOOST_REQUIRE_EQUAL(e0.size(), 1);
    BOOST_REQUIRE_EQUAL(e0[0]->targets.front(), c1);

    auto e1 = dg.successors(dg.get_config(1));
    BOOST_REQUIRE_EQUAL(e1.size(), 1);
    BOOST_REQUIRE_EQUAL(e1[0]->targets.front(), c2);

    auto e2 = dg.successors(dg.get_config(2));
    BOOST_REQUIRE_EQUAL(e2.size(), 3);
    BOOST_REQUIRE_EQUAL(e2[0]->targets.front(), c0);

    BOOST_REQUIRE(std::find(std::begin(e2[1]->targets), std::end(e2[1]->targets), c0) != std::end(e2[1]->targets));
    BOOST_REQUIRE(std::find(std::begin(e2[1]->targets), std::end(e2[1]->targets), c3) != std::end(e2[1]->targets));

    BOOST_REQUIRE_EQUAL(e2[2]->targets.front(), c3);

    auto e3 = dg.successors(dg.get_config(3));
    BOOST_REQUIRE_EQUAL(e3.size(), 2);
    BOOST_REQUIRE(e3[0]->targets.empty());

    BOOST_REQUIRE_EQUAL(e3[1]->targets.front(), c2);

    BOOST_REQUIRE(alg.search(dg));
}

BOOST_AUTO_TEST_CASE(ManualDgAssignment) {
    std::stringstream ss{
            std::string{"0 1\n"} +
            "1 2\n"
            "2 0\n"
            "2 0 3\n"
            "2 3\n"
            "3 4\n"
            "3 2\n"
            "# 4 0"};
    auto dg = DependencyGraph::parse_dg<int>(ss);

    CertainZeroFPA alg{Strategy::DFS};
    BOOST_REQUIRE(!alg.search(dg));

    dg.reset_state();
    dg.set_assignment(4, 1);
    BOOST_REQUIRE(alg.search(dg));
}

BOOST_AUTO_TEST_CASE(ManualDgNegation) {
    std::stringstream ss{
        std::string{"0 1 2\n"}
        + "! 0 3\n"
    };
    auto dg = DependencyGraph::parse_dg<int>(ss);
    CertainZeroFPA alg{Strategy::DFS};
    BOOST_REQUIRE(alg.search(dg));
}



