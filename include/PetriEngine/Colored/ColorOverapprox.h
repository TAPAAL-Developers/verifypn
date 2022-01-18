/* Copyright (C) 2020  Peter G. Jensen <root@petergjoel.dk>,
 *                     Alexander Bilgram <alexander@bilgram.dk>,
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

/*
 * File:   ColorOverapprox.h
 * Author: Peter G. Jensen
 *
 * Created on 18 January 2022, 10.16
 */

#ifndef COLOROVERAPPROX_H
#define COLOROVERAPPROX_H

#include "IntervalGenerator.h"


#include <unordered_map>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    class ColorOverapprox {
    public:
        using color_var_map_t = std::unordered_map<const Colored::Variable *, Colored::interval_vector_t>;
        using color_map_vector_t = std::vector<color_var_map_t>;
        using color_map_t = std::vector<color_map_vector_t>;

        ColorOverapprox(ColoredPetriNetBuilder& builder);
        void compute(uint32_t maxIntervals, uint32_t maxIntervalsReduced, int32_t timeout);
        double time() const {
            return _fixPointCreationTime;
        }

        bool computed() const {
            return _fixpointDone;
        }

        const std::vector<Colored::ColorFixpoint>& places_fixpoint() const {
            return _placeColorFixpoints;
        }

        std::unordered_map<uint32_t, Colored::ArcIntervals> default_transition_intervals(const Colored::Transition &transition) const;

        uint64_t max_intervals() const { return _max_intervals; }
        color_map_t& variable_maps() {
            return _var_map;
        }

        void print() const;
    private:
        Colored::ColorFixpoint default_place_fixpoint(uint32_t pid) const;
        bool process_input_arcs(size_t transition_id, uint32_t currentPlaceId, uint32_t max_intervals);
        void process_output_arcs(size_t transition_id);
        void add_transition_vars(size_t transition_id);
        void removeInvalidVarmaps(size_t transition_id);
        void init_place(size_t placeid);
        bool make_arc_intervals(uint32_t transitionId, uint32_t max_intervals);

        ColoredPetriNetBuilder& _builder;
        std::vector<Colored::ColorFixpoint> _placeColorFixpoints;
        bool _fixpointDone = false;
        std::vector<uint32_t> _placeFixpointQueue;
        double _fixPointCreationTime;
        std::unordered_map<uint32_t, std::unordered_map<uint32_t, Colored::ArcIntervals>> _arcIntervals;
        const Colored::IntervalGenerator _intervalGenerator = Colored::IntervalGenerator();
        uint64_t _max_intervals = 0;
        std::vector<color_map_vector_t> _var_map;
        std::vector<bool> _considered;

    };
}

#endif /* COLOROVERAPPROX_H */

