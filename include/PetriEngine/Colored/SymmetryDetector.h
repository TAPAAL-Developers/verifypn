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
 * File:   SymmetryDetector.h
 * Author: Peter G. Jensen
 *
 * Created on 18 January 2022, 15.31
 */

#ifndef SYMMETRYDETECTOR_H
#define SYMMETRYDETECTOR_H
#include "ColoredNetStructures.h"


#include <set>

namespace PetriEngine {
    class ColoredPetriNetBuilder;
    class SymmetryDetector {
    public:
        SymmetryDetector(const ColoredPetriNetBuilder& builder);
        void compute();
        void print() const;
        const std::vector<std::set<const Colored::Variable *>>& operator[](size_t tid) const
        {
            return _symmetric[tid];
        }
    private:
        bool check_in_arcs(const Colored::Transition &transition, const Colored::Arc &inArc, const std::set<const Colored::Variable*> &inArcVars) const;
        bool check_out_arcs(const Colored::Transition &transition, const std::set<const Colored::Variable*> &inArcVars) const;
        const ColoredPetriNetBuilder& _builder;
            //transition id to vector of vectors of variables, where variable in vector are symmetric
        std::vector<std::vector<std::set<const Colored::Variable *>>> _symmetric;
    };
}

#endif /* SYMMETRYDETECTOR_H */

