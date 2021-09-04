/* Copyright (C) 2020  Alexander Bilgram <alexander@bilgram.dk>,
 *                     Peter Haar Taankvist <ptaankvist@gmail.com>,
 *                     Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                     Andreas H. Klostergaard
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

#ifndef COLOREDPETRINETBUILDER_H
#define COLOREDPETRINETBUILDER_H

#include <vector>
#include <unordered_map>

#include "../AbstractPetriNetBuilder.h"
#include "../PetriNetBuilder.h"
#include "BindingGenerator.h"
#include "IntervalGenerator.h"
#include "PartitionBuilder.h"
#include "ArcIntervals.h"

namespace PetriEngine {

    class ColoredPetriNetBuilder : public AbstractPetriNetBuilder {
    public:
        typedef std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>> PTPlaceMap;
        typedef std::unordered_map<std::string, std::vector<std::string>> PTTransitionMap;

    public:
        ColoredPetriNetBuilder();
        ColoredPetriNetBuilder(const ColoredPetriNetBuilder& orig);
        virtual ~ColoredPetriNetBuilder();

        void add_place(const std::string& name,
                int tokens,
                double x = 0,
                double y = 0) override;
        void add_place(const std::string& name,
                const Colored::ColorType* type,
                Colored::Multiset&& tokens,
                double x = 0,
                double y = 0) override;
        void add_transition(const std::string& name,
                double x = 0,
                double y = 0) override;
        void add_transition(const std::string& name,
                const Colored::GuardExpression_ptr& guard,
                double x = 0,
                double y = 0) override;
        void add_input_arc(const std::string& place,
                const std::string& transition,
                bool inhibitor,
                int) override;
        void add_input_arc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool inhibitor, int weight) override;
        void add_output_arc(const std::string& transition,
                const std::string& place,
                int weight = 1) override;
        void add_output_arc(const std::string& transition,
                const std::string& place,
                const Colored::ArcExpression_ptr& expr) override;
        void add_color_type(const std::string& id,
                const Colored::ColorType* type) override;


        void sort() override;

        double get_unfold_time() const {
            return _time;
        }

        double get_partition_time() const {
            return _partitionTimer;
        }

        double get_fixpoint_time() const {
            return _fixPointCreationTime;
        }

        uint32_t get_place_count() const {
            return _places.size();
        }

        uint32_t get_max_intervals() const {
            return _maxIntervals;
        }

        uint32_t get_transition_count() const {
            return _transitions.size();
        }

        uint32_t get_arc_count() const {
            uint32_t sum = 0;
            for (auto& t : _transitions) {
                sum += t._input_arcs.size();
                sum += t._output_arcs.size();
            }
            return sum;
        }

        uint32_t get_unfolded_place_count() const {
            return _ptBuilder.number_of_places();
        }

        uint32_t get_unfolded_transition_count() const {
            return _ptBuilder.number_of_transitions();
        }

        uint32_t get_unfolded_arc_count() const {
            return _nptarcs;
        }

        bool is_unfolded() const {
            return _unfolded;
        }

        const PTPlaceMap& get_unfolded_place_names() const {
            return _ptplacenames;
        }

        const PTTransitionMap& get_unfolded_transition_names() const {
            return _pttransitionnames;
        }

        PetriNetBuilder& unfold();
        PetriNetBuilder& strip_colors();
        void compute_place_color_fixpoint(uint32_t max_intervals, uint32_t max_intervals_reduced, int32_t timeout);
        void compute_partition(int32_t timeout);
        void compute_symmetric_variables();
        void print_symmetric_variables() const;

    private:
        std::unordered_map<std::string,uint32_t> _placenames;
        std::unordered_map<std::string,uint32_t> _transitionnames;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
        std::unordered_map<uint32_t,std::vector<uint32_t>> _placePostTransitionMap;
        std::unordered_map<uint32_t,std::vector<uint32_t>> _placePreTransitionMap;
        std::unordered_map<uint32_t,FixpointBindingGenerator> _bindings;
        PTPlaceMap _ptplacenames;
        PTTransitionMap _pttransitionnames;
        uint32_t _nptarcs = 0;
        uint32_t _maxIntervals = 0;
        const Colored::IntervalGenerator _intervalGenerator = Colored::IntervalGenerator();

        std::vector<Colored::Place> _places;
        std::vector<Colored::Transition> _transitions;
        std::vector<Colored::Arc> _inhibitorArcs;
        std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
        //transition id to vector of vectors of variables, where variable in vector are symmetric
        std::unordered_map<uint32_t, std::vector<std::set<const Colored::Variable *>>> _symmetric_var_map;

        std::unordered_map<uint32_t, std::string> _sumPlacesNames;
        Colored::ColorTypeMap _colors;
        PetriNetBuilder _ptBuilder;
        bool _unfolded = false;
        bool _stripped = false;
        bool _fixpointDone = false;
        bool _partitionComputed = false;

        std::vector<uint32_t> _placeFixpointQueue;
        std::unordered_map<uint32_t, Colored::EquivalenceVec> _partition;

        double _time;
        double _fixPointCreationTime;

        double _partitionTimer = 0;

        std::string arc_to_string(const Colored::Arc& arc) const;

        void print_place_table() const;

        void check_symmetric_vars_in_arcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const;
        void check_symmetric_vars_out_arcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars, bool &isEligible) const;
        void remove_invalid_varmaps(Colored::Transition& transition) const;
        void add_transition_vars(Colored::Transition& transition) const;

        std::unordered_map<uint32_t, Colored::ArcIntervals> setup_transition_vars(const Colored::Transition &transition) const;

        void add_arc(const std::string& place,
                const std::string& transition,
                const Colored::ArcExpression_ptr& expr,
                bool input, bool inhibitor, int weight);

        void find_stable_places();

        void get_arc_intervals(const Colored::Transition& transition, bool &transitionActivated, uint32_t max_intervals, uint32_t transitionId);
        void process_input_arcs(Colored::Transition& transition, uint32_t currentPlaceId, uint32_t transitionId, bool &transitionActivated, uint32_t max_intervals);
        void process_output_arcs(Colored::Transition& transition);

        void unfold_place(const Colored::Place* place, const PetriEngine::Colored::Color *color, uint32_t unfoldPlace, uint32_t id);
        void unfold_transition(uint32_t transitionId);
        void handle_orphan_place(const Colored::Place& place, const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap);
        void create_partion_varmaps();
        void unfold_inhibitor_arc(const std::string &oldname, const std::string &newname);

        void unfold_arc(const Colored::Arc& arc, const Colored::BindingMap& binding, const std::string& name);
    };

    //Used for checking if a variable is inside either a succ or pred expression
    enum expression_type_e {
        None,
        Pred,
        Succ
    };


}

#endif /* COLOREDPETRINETBUILDER_H */
