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

#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include "AbstractPetriNetBuilder.h"
#include "PQL/PQL.h"
#include "PetriNet.h"
#include "Reducer.h"
#include "NetStructures.h"
#include "Reachability/ReachabilityResult.h"
namespace PetriEngine {
    /** Builder for building engine representations of PetriNets */
    class PetriNetBuilder : public AbstractPetriNetBuilder {
    public:
        friend class Reducer;

    public:
        PetriNetBuilder();
        PetriNetBuilder(const PetriNetBuilder& other);
        void add_place(const std::string& name, int tokens, double x, double y) override;
        void add_transition(const std::string& name,
                double x,
                double y) override;
        void add_input_arc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int weight) override;
        void add_output_arc(const std::string& transition, const std::string& place, int weight) override;

        virtual void sort() override;
        /** Make the resulting petri net, you take ownership */
        PetriNet* make_petri_net(bool reorder = true);
        /** Make the resulting initial marking, you take ownership */

        MarkVal const * init_marking()
{
            return _initial_marking.data();
        }

        uint32_t number_of_places() const
        {
            return _placenames.size();
        }

        uint32_t number_of_transitions() const
        {
            return _transitionnames.size();
        }

        const std::unordered_map<std::string, uint32_t>& get_place_names() const
        {
            return _placenames;
        }

        const std::unordered_map<std::string, uint32_t>& get_transition_names() const
        {
            return _transitionnames;
        }

        void reduce(std::vector<std::shared_ptr<PQL::Condition> >& query,
                    std::vector<Reachability::ResultPrinter::Result>& results,
                    int reductiontype, bool reconstructTrace, const PetriNet* net, int timeout, std::vector<uint32_t>& reductions);

        size_t removed_transitions() const {
            return _reducer.removed_transitions();
        }

        size_t removed_places() const {
            return _reducer.removed_places();
        }

        void print_stats(std::ostream& out) {
            _reducer.print_stats(out);
        }

        Reducer* get_reducer() {
            return &_reducer;
        }

        std::vector<std::pair<std::string, uint32_t>> orphan_places() const {
            std::vector<std::pair<std::string, uint32_t>> res;
            for(uint32_t p = 0; p < _places.size(); p++) {
                if(_places[p].consumers.size() == 0 && _places[p].producers.size() == 0) {
                    for(auto &n : _placenames) {
                        if(n.second == p) {
                            res.push_back(std::make_pair(n.first, _initial_marking[p]));
                            break;
                        }
                    }
                }
            }
            return res;
        }

        double reduction_time() const {
            // duration in seconds
            auto end = std::chrono::high_resolution_clock::now();
            return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
        }

        void start_timer() {
            _start = std::chrono::high_resolution_clock::now();
        }

    private:
        uint32_t next_place_id(std::vector<uint32_t>& counts,  std::vector<uint32_t>& pcounts, std::vector<uint32_t>& ids, bool reorder);
        std::chrono::high_resolution_clock::time_point _start;

    protected:
        std::unordered_map<std::string, uint32_t> _placenames;
        std::unordered_map<std::string, uint32_t> _transitionnames;

        std::vector< std::tuple<double, double> > _placelocations;
        std::vector< std::tuple<double, double> > _transitionlocations;

        std::vector<PetriEngine::Transition> _transitions;
        std::vector<PetriEngine::Place> _places;

        std::vector<MarkVal> _initial_marking;
        Reducer _reducer;
    };

}

#endif // PETRINETBUILDER_H

