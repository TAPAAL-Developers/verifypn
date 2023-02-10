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


#ifndef VERIFYPN_ENUMERATINGPETRINETBUILDER_H
#define VERIFYPN_ENUMERATINGPETRINETBUILDER_H

#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"

class EnumeratingPetriNetBuilder : public PetriEngine::PetriNetBuilder {
public:
    explicit EnumeratingPetriNetBuilder(const PetriEngine::PetriNetBuilder &pn_builder) : PetriNetBuilder(pn_builder) {}

    explicit EnumeratingPetriNetBuilder(shared_string_set& stringSet) : PetriNetBuilder(stringSet) {}

    std::vector<PetriEngine::Transition> transitions() const { return _transitions; }
    std::vector<PetriEngine::Place> places() const { return _places; }

private:

};


#endif //VERIFYPN_ENUMERATINGPETRINETBUILDER_H
