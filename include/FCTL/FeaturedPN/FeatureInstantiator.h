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


#ifndef VERIFYPN_FEATUREINSTANTIATOR_H
#define VERIFYPN_FEATUREINSTANTIATOR_H

#include "EnumeratingPetriNetBuilder.h"

class FeatureInstantiator {
public:
    explicit FeatureInstantiator(const PetriEngine::PetriNetBuilder& builder) : builder_(builder) {}

    template<typename Pred>
    [[nodiscard]] PetriEngine::PetriNet* make_stripped_petri_net(Pred&& trans_valid) {
        auto transitions = builder_.transitions();
        auto places = builder_.places();
        for (auto& t: transitions) {
            if (t.feature == bddfalse || t.feature == bddtrue) continue;
            t.skip = trans_valid(t);
        }
        return builder_.makePetriNet(false);
    }

private:
    EnumeratingPetriNetBuilder builder_;
};



#endif //VERIFYPN_FEATUREINSTANTIATOR_H
