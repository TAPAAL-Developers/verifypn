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

template <typename T>
void reset_dg(ManualDG<T> &dg) {
    dg.reset_state();
}


template <typename T, typename StateReset>
void test_all(ManualDG<T> &dg, bool expected, StateReset &&reset_state) {
    LocalFPA local{Strategy::DFS};
    BOOST_REQUIRE(local.search(dg) == expected);
    reset_state(dg);

    CertainZeroFPA czero{Strategy::DFS};
    BOOST_REQUIRE(czero.search(dg) == expected);
}

template <typename T>
void test_all(ManualDG<T> &dg, bool expected) {
    // need lambda to wrap reset_dg, idk why
    test_all(dg, expected, [](ManualDG<T> &dg) { reset_dg(dg); });
}


BOOST_AUTO_TEST_CASE(DirectoryTest) {
        BOOST_REQUIRE(getenv("TEST_FILES"));
}

BOOST_AUTO_TEST_CASE(TestTest) {
    // ELS:SPIN:19, Fig. 1. expects true
    DependencyGraph::ManualDG<int> dg;
    dg.add_hyperedge(0, {1});
    dg.add_hyperedge(1, {2});
    dg.add_hyperedge(2, {0});
    dg.add_hyperedge(2, {0, 3});
    dg.add_hyperedge(2, {3});
    dg.add_hyperedge(3, {});
    dg.add_hyperedge(3, {2});

    LocalFPA alg{Strategy::DFS};
    test_all(dg, true);
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

    test_all(dg, true);
}

BOOST_AUTO_TEST_CASE(ManualDgAssignment) {
    std::string s{
            "0 1\n"
            "1 2\n"
            "2 0\n"
            "2 0 3\n"
            "2 3\n"
            "3 4\n"
            "3 2\n"
            "# 4 0"};
    std::stringstream ss{s};
    auto dg = DependencyGraph::parse_dg<int>(ss);

    CertainZeroFPA alg{Strategy::DFS};
    test_all(dg, false);
}

BOOST_AUTO_TEST_CASE(ManualDgAssignTrue) {
    std::string s{
            "0 1\n"
            "1 2\n"
            "2 0\n"
            "2 0 3\n"
            "2 3\n"
            "3 4\n"
            "3 2\n"
            "# 4 1"};
    std::stringstream ss{s};
    auto dg = DependencyGraph::parse_dg<int>(ss);

    dg.set_assignment(4, 1);
    test_all(dg, true, [](ManualDG<int> &dg) { dg.reset_state(); dg.set_assignment(4, 1); });
}

BOOST_AUTO_TEST_CASE(ManualDgNegation) {
    std::stringstream ss{
        std::string{"0 1 2\n"}
        + "! 0 3\n"
    };
    auto dg = DependencyGraph::parse_dg<int>(ss);
    CertainZeroFPA alg{Strategy::DFS};
    test_all(dg, true);
}

BOOST_AUTO_TEST_CASE(ExamplesFromPapers) {
    // ELS:SPIN:19, fig. 2. expects 1
    std::string s0{R"(
1 2
1 3 4 7
3
4 5
4 6 7
6
7 1 9
7 8
8
)"};
    // ELS:SPIN:19, fig. 3. expects 0
    std::string s1{R"(
1 2
1 3 4
3
4 5 6
4 7
7 4
5 6
6 4 5
)"};
    // ELS:SPIN:19, fig. 6. expects 1
    std::string s2{R"(
1 2
1 5
1 8
2 3
2 4
4 6
6
5 6 7
8 7
8 9 10 11
)"};
    // DEFJJJKLNOPS:FI:18, fig. 1. expects 0
    std::string s3{R"(
1 2 4
! 1 5
5 4 6
2 3
3 2
! 4 3
6
)"};
    // DEFJJJKLNOPS:FI:18, fig. 6. expects 1
    std::string s4{R"(
0 1
0 2 3
! 2 4
4 8
! 8 11
11
4 9
9 12
9 13
13 15
! 15 16
16
! 12 14
14
3 5
3 6 7
! 6 9
7 10
10
)"};

    CertainZeroFPA cz{Strategy::DFS};

    auto dg0 = parse_dg<int>(s0);
    auto dg1 = parse_dg<int>(s1);
    auto dg2 = parse_dg<int>(s2);
    auto dg3 = parse_dg<int>(s3);
    auto dg4 = parse_dg<int>(s4);
    BOOST_REQUIRE(cz.search(dg0) == true);
    BOOST_REQUIRE(cz.search(dg1) == false);
    BOOST_REQUIRE(cz.search(dg2) == true);
    BOOST_REQUIRE(cz.search(dg3) == false);
    BOOST_REQUIRE(cz.search(dg4) == true);

}
