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

#include <unordered_map>
#include <vector>

#include "../AbstractPetriNetBuilder.h"
#include "../PetriNetBuilder.h"
#include "ArcIntervals.h"
#include "BindingGenerator.h"
#include "IntervalGenerator.h"
#include "PartitionBuilder.h"

namespace PetriEngine {

class ColoredPetriNetBuilder : public AbstractPetriNetBuilder {
  public:
    using PTPlaceMap = std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>>;
    using PTTransitionMap = std::unordered_map<std::string, std::vector<std::string>>;

  public:
    ColoredPetriNetBuilder();
    ColoredPetriNetBuilder(const ColoredPetriNetBuilder &orig);
    ~ColoredPetriNetBuilder() override;

    void add_place(const std::string &name, uint32_t tokens, double x = 0, double y = 0) override;
    void add_place(const std::string &name, const Colored::ColorType *type,
                   Colored::Multiset &&tokens, double x = 0, double y = 0) override;
    void add_transition(const std::string &name, double x = 0, double y = 0) override;
    void add_transition(const std::string &name, const Colored::GuardExpression_ptr &guard,
                        double x = 0, double y = 0) override;
    void add_input_arc(const std::string &place, const std::string &transition, bool inhibitor,
                       uint32_t) override;
    void add_input_arc(const std::string &place, const std::string &transition,
                       const Colored::ArcExpression_ptr &expr, bool inhibitor,
                       uint32_t weight) override;
    void add_output_arc(const std::string &transition, const std::string &place,
                        uint32_t weight = 1) override;
    void add_output_arc(const std::string &transition, const std::string &place,
                        const Colored::ArcExpression_ptr &expr) override;
    void add_color_type(const std::string &id, const Colored::ColorType *type) override;

    void sort() override;

    auto get_unfold_time() const -> double { return _time; }

    auto get_partition_time() const -> double { return _partitionTimer; }

    auto get_fixpoint_time() const -> double { return _fixPointCreationTime; }

    auto get_place_count() const -> uint32_t { return _places.size(); }

    auto get_max_intervals() const -> uint32_t { return _maxIntervals; }

    auto get_transition_count() const -> uint32_t { return _transitions.size(); }

    auto get_arc_count() const -> uint32_t {
        uint32_t sum = 0;
        for (auto &t : _transitions) {
            sum += t._input_arcs.size();
            sum += t._output_arcs.size();
        }
        return sum;
    }

    auto get_unfolded_place_count() const -> uint32_t { return _ptBuilder.number_of_places(); }

    auto get_unfolded_transition_count() const -> uint32_t {
        return _ptBuilder.number_of_transitions();
    }

    auto get_unfolded_arc_count() const -> uint32_t { return _nptarcs; }

    auto is_unfolded() const -> bool { return _unfolded; }

    auto get_unfolded_place_names() const -> const PTPlaceMap & { return _ptplacenames; }

    auto get_unfolded_transition_names() const -> const PTTransitionMap & {
        return _pttransitionnames;
    }

    auto unfold() -> PetriNetBuilder &;
    auto strip_colors() -> PetriNetBuilder &;
    void compute_place_color_fixpoint(uint32_t max_intervals, uint32_t max_intervals_reduced,
                                      int32_t timeout);
    void compute_partition(int32_t timeout);
    void compute_symmetric_variables();
    void print_symmetric_variables() const;

  private:
    std::unordered_map<std::string, uint32_t> _placenames;
    std::unordered_map<std::string, uint32_t> _transitionnames;
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, Colored::arc_intervals_t>>
        _arcIntervals;
    std::unordered_map<uint32_t, std::vector<uint32_t>> _placePostTransitionMap;
    std::unordered_map<uint32_t, std::vector<uint32_t>> _placePreTransitionMap;
    PTPlaceMap _ptplacenames;
    PTTransitionMap _pttransitionnames;
    uint32_t _nptarcs = 0;
    uint32_t _maxIntervals = 0;
    const Colored::IntervalGenerator _intervalGenerator = Colored::IntervalGenerator();

    std::vector<Colored::place_t> _places;
    std::vector<Colored::transition_t> _transitions;
    std::vector<Colored::arc_t> _inhibitorArcs;
    std::vector<Colored::color_fixpoint_t> _placeColorFixpoints;
    // transition id to vector of vectors of variables, where variable in vector are symmetric
    std::unordered_map<uint32_t, std::vector<std::set<const Colored::Variable *>>>
        _symmetric_var_map;

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

    auto arc_to_string(const Colored::arc_t &arc) const -> std::string;

    void print_place_table() const;

    void check_symmetric_vars_in_arcs(const Colored::transition_t &transition,
                                      const Colored::arc_t &inArc,
                                      const std::set<const Colored::Variable *> &inArcVars,
                                      bool &isEligible) const;
    void check_symmetric_vars_out_arcs(const Colored::transition_t &transition,
                                       const std::set<const Colored::Variable *> &inArcVars,
                                       bool &isEligible) const;
    void remove_invalid_varmaps(Colored::transition_t &transition) const;
    void add_transition_vars(Colored::transition_t &transition) const;

    auto setup_transition_vars(const Colored::transition_t &transition) const
        -> std::unordered_map<uint32_t, Colored::arc_intervals_t>;

    void add_arc(const std::string &place, const std::string &transition,
                 const Colored::ArcExpression_ptr &expr, bool input, bool inhibitor, int weight);

    void find_stable_places();

    void get_arc_intervals(const Colored::transition_t &transition, bool &transitionActivated,
                           uint32_t max_intervals, uint32_t transitionId);
    void process_input_arcs(Colored::transition_t &transition, uint32_t currentPlaceId,
                            uint32_t transitionId, bool &transitionActivated,
                            uint32_t max_intervals);
    void process_output_arcs(Colored::transition_t &transition);

    void unfold_place(const Colored::place_t *place, const PetriEngine::Colored::Color *color,
                      uint32_t unfoldPlace, uint32_t id);
    void unfold_transition(uint32_t transitionId);
    void handle_orphan_place(const Colored::place_t &place,
                             const std::unordered_map<std::string, uint32_t> &unfoldedPlaceMap);
    void create_partion_varmaps();
    void unfold_inhibitor_arc(const std::string &oldname, const std::string &newname);

    void unfold_arc(const Colored::arc_t &arc, const Colored::BindingMap &binding,
                    const std::string &name);
};

// Used for checking if a variable is inside either a succ or pred expression
enum expression_type_e { NONE, PRED, SUCC };

} // namespace PetriEngine

#endif /* COLOREDPETRINETBUILDER_H */
