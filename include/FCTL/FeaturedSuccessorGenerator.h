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


#ifndef VERIFYPN_FEATUREDSUCCESSORGENERATOR_H
#define VERIFYPN_FEATUREDSUCCESSORGENERATOR_H

#include "PetriEngine/SuccessorGenerator.h"
#include "PetriEngine/PQL/PQL.h"
namespace Featured {

    class FeaturedSuccessorGenerator : public PetriEngine::SuccessorGenerator {
    public:
        FeaturedSuccessorGenerator(const PetriEngine::PetriNet& net, const std::shared_ptr<PetriEngine::PQL::Condition>& query)
                : SuccessorGenerator(net, query) {}


        explicit FeaturedSuccessorGenerator(const PetriEngine::PetriNet& net) : SuccessorGenerator(net) {}

        [[nodiscard]] bdd feature() const {
            return _net.feat(_suc_tcounter);
        }
    };

} // Featured

#endif //VERIFYPN_FEATUREDSUCCESSORGENERATOR_H
