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


#ifndef VERIFYPN_MANUALDG_H
#define VERIFYPN_MANUALDG_H

#include "CTL/DependencyGraph/BasicDependencyGraph.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "utils/errors.h"

#include <unordered_map>
#include <iostream>

namespace DependencyGraph {
    /**
     * NOTE: Mainly intended for debug/testing, and might not be memory safe.
     *
     * Class for building a desired dependency graph by hand. Currently this is a add-only process,
     * deletion would require memory fiddling unless you don't mind memory leaks.
     * A ManualDG can be constructed either using parse_dg(std::istream&), which
     * compiles a line-based text format to a DG, or by manually calling add_hyperedge and set_assignment.
     * Either way configurations are specified by ID (default type string).
     * @tparam IDType Type of identifiers used to manage configurations. Defaults to string,
     *                but manual building might be nicer using int.
     */
    template<typename IDType = std::string>
    class ManualDG : public BasicDependencyGraph {
    public:
        std::vector<Edge*> successors(Configuration* c) override {
            return edges_[c]; // using map::operator[] so empty vector is returned if no edges exist.
        }

        Configuration* initialConfiguration() override {
            assert(root_ != nullptr);
            return root_;
        }

        void release(Edge* e) override {}

        void cleanUp() override {}

        void set_assignment(const IDType& id, int i) {
            get_config(id)->assignment = to_assignment(i);
        }

        void add_hyperedge(const IDType& id, const std::vector<IDType>& sucs) {
            auto* conf = get_config(id);
            if (root_ == nullptr) {
                root_ = conf;
            }
            auto* e = new Edge{*conf};
            e->refcnt = 1;
            for (auto& s: sucs) {
                // this allows edge e.g. (c, {x, y, x}), but the duplicated target shouldn't cause issues
                e->addTarget(get_config(s));
            }
            edges_[conf].push_back(e);
        }

        void add_negation(const IDType& source, const IDType& target) {
            auto s = get_config(source), t = get_config(target);
            if (root_ == nullptr) {
                root_ = s;
            }
            auto *e = new Edge{*s};
            e->refcnt = 1;
            e->addTarget(t);
            e->is_negated = true;
            edges_[s].push_back(e);
        }

        virtual ~ManualDG() {
            // this _should_ catch everything
            // - new Configuration is only in get_config which adds to configs_,
            // - new Edge is only in add_hyperedge which pushes it to edges_.
            for (auto& [_, es]: edges_) {
                for (auto* e: es) {
                    delete e;
                }
            }
            for (auto& [_, c]: configs_) {
                delete c;
            }
        }

        //void print_to_dot(std::ostream &os); // could be nice maybe?

        Configuration* get_config(const IDType& id) {
            if (auto it = configs_.find(id); it != std::end(configs_)) {
                return it->second;
            }
            // else create the new config
            auto* c = new Configuration{};
            configs_[id] = c;
            return c;
        }

        void reset_state() {
            for (auto &[c, es] : edges_) {
                for (auto* e : es) {
                    e->handled = false;
                    e->processed = false;
                    e->refcnt = 1; e->status = 0;//EdgeStatus::NotWaiting;
                }
            }
            for (auto &[_, c] : configs_) {
                c->nsuccs = 0;
                //c->distance = 0;
                c->assignment = UNKNOWN;
                c->dependency_set.clear();
            }
        }

        void print(Configuration* config, std::ostream& out) override {}

    private:

        std::unordered_map<Configuration*, std::vector<Edge*>> edges_;
        std::unordered_map<IDType, Configuration*> configs_;
        Configuration* root_ = nullptr;

        Assignment to_assignment(int i) const {
            switch (i) {
                case 0:
                    return CZERO;
                case 1:
                    return ONE;
                default:
                    throw base_error{"Invalid value " + std::to_string(i) + " for assignment. Expected '0' or '1'"};
            }
        }

    };

    /**
     * Construct a fixed dependency graph from a structured text format.
     * The format should be a list of lines of form either
     *   - c x1 x2 ... xn
     *     says to add hyperedge (c, {x1, x2, ..., xn})
     *   - # c x
     *     says the assignment of c is x (x should be one of {0, 1})
     *   where c, x, xi are arbitrary strings (except #).
     *   Source of first hyperedge is taken to be the root node.
     * @param is  source of text input. Is wholly consumed by the function.
     * @return    the dependency graph described by the text given by is.
     */
    template <typename T>
    void parse_assign_(const std::string& s, ManualDG<T>& graph) {
        std::istringstream is{s};
        T id;
        int assignment;
        is.get();
        //is >> id; // skip '#'
        is >> id >> std::ws;
        is >> assignment;
        if (!is) {
            throw base_error{"Malformed assignment statement " + s};
        }
        graph.set_assignment(id, assignment);
    }

    template <typename T>
    void parse_negation_(const std::string& s, ManualDG<T>& graph) {
        std::istringstream is{s};
        T source, target;
        is.get(); // skip '!'
        //is >> source;
        is >> source >> std::ws >> target;
        if (!is) {
            throw base_error{"Malformed negation edge specification " + s};
        }
        graph.add_negation(source, target);
    }

    template <typename T>
    void parse_hyperedge_(const std::string& s, ManualDG<T>& graph) {
        std::istringstream is{s};
        T id;
        is >> id >> std::ws;
        T suc;
        std::vector<T> sucs;
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
    template <typename T>
    ManualDG<T> parse_dg(std::istream& is) {
        ManualDG<T> dg;

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

    template <typename T>
    ManualDG<T> parse_dg(const std::string& s) {
        std::stringstream ss{s};
        return parse_dg<T>(ss);
    }

    template <typename T>
    ManualDG<T> parse_dg(const char* s) {
        std::stringstream ss{s};
        return parse_dg<T>(ss);
    }
}

#endif //VERIFYPN_MANUALDG_H
