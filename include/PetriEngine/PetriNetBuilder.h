/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
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
#ifndef PETRINETBUILDER_H
#define PETRINETBUILDER_H

#include "AbstractPetriNetBuilder.h"
#include "NetStructures.h"
#include "PQL/PQL.h"
#include "PetriNet.h"
#include "Reachability/ReachabilityResult.h"
#include "Reducer.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
namespace PetriEngine {
/** Builder for building engine representations of PetriNets */
class PetriNetBuilder : public AbstractPetriNetBuilder {
  public:
    friend class Reducer;

  public:
    PetriNetBuilder();
    PetriNetBuilder(const PetriNetBuilder &other);
    void add_place(const std::string &name, uint32_t tokens, double x, double y) override;
    void add_transition(const std::string &name, double x, double y) override;
    void add_input_arc(const std::string &place, const std::string &transition, bool inhibitor,
                       uint32_t weight) override;
    void add_output_arc(const std::string &transition, const std::string &place,
                        uint32_t weight) override;

    void sort() override;
    /** Make the resulting petri net, you take ownership */
    auto make_petri_net(bool reorder = true) -> PetriNet *;
    /** Make the resulting initial marking, you take ownership */

    auto init_marking() -> MarkVal const * { return _initial_marking.data(); }

    auto number_of_places() const -> uint32_t { return _placenames.size(); }

    auto number_of_transitions() const -> uint32_t { return _transitionnames.size(); }

    auto get_place_names() const -> const std::unordered_map<std::string, uint32_t> & {
        return _placenames;
    }

    auto get_transition_names() const -> const std::unordered_map<std::string, uint32_t> & {
        return _transitionnames;
    }

    void reduce(std::vector<std::shared_ptr<PQL::Condition>> &query,
                std::vector<Reachability::ResultPrinter::Result> &results, int reductiontype,
                bool reconstructTrace, const PetriNet *net, int timeout,
                std::vector<uint32_t> &reductions);

    auto removed_transitions() const -> size_t { return _reducer.removed_transitions(); }

    auto removed_places() const -> size_t { return _reducer.removed_places(); }

    void print_stats(std::ostream &out) { _reducer.print_stats(out); }

    auto get_reducer() -> Reducer & { return _reducer; }

    auto get_reducer() const -> const Reducer & { return _reducer; }

    auto orphan_places() const -> std::vector<std::pair<std::string, uint32_t>> {
        std::vector<std::pair<std::string, uint32_t>> res;
        for (uint32_t p = 0; p < _places.size(); p++) {
            if (_places[p]._consumers.size() == 0 && _places[p]._producers.size() == 0) {
                for (auto &n : _placenames) {
                    if (n.second == p) {
                        res.push_back(std::make_pair(n.first, _initial_marking[p]));
                        break;
                    }
                }
            }
        }
        return res;
    }

    auto reduction_time() const -> double {
        // duration in seconds
        auto end = std::chrono::high_resolution_clock::now();
        return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count()) *
               0.000001;
    }

    void start_timer() { _start = std::chrono::high_resolution_clock::now(); }

  private:
    auto next_place_id(std::vector<uint32_t> &counts, std::vector<uint32_t> &pcounts,
                       std::vector<uint32_t> &ids, bool reorder) -> uint32_t;
    std::chrono::high_resolution_clock::time_point _start;

  protected:
    std::unordered_map<std::string, uint32_t> _placenames;
    std::unordered_map<std::string, uint32_t> _transitionnames;

    std::vector<std::tuple<double, double>> _placelocations;
    std::vector<std::tuple<double, double>> _transitionlocations;

    std::vector<PetriEngine::transition_t> _transitions;
    std::vector<PetriEngine::place_t> _places;

    std::vector<MarkVal> _initial_marking;
    Reducer _reducer;
};

} // namespace PetriEngine

#endif // PETRINETBUILDER_H
