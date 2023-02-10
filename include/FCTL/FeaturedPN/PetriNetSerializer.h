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


#ifndef VERIFYPN_PETRINETSERIALIZER_H
#define VERIFYPN_PETRINETSERIALIZER_H

#include "PetriEngine/PetriNet.h"
#include "FCTL/FeaturedPN/EnumeratingPetriNetBuilder.h"

#include <filesystem>

class PetriNetSerializer {
public:
    explicit PetriNetSerializer(PetriEngine::PetriNetBuilder& builder, bool verbose);

    void serialize_features(const std::filesystem::path &odir);

private:

    EnumeratingPetriNetBuilder builder_;

    template <typename T>
    void enumerate_rec(const std::vector<T>& vals, std::vector<std::vector<T>>& sets, std::vector<bool>& current_set, int depth);

    template <typename T>
    std::vector<std::vector<T>> enumerate(const std::vector<T>& vals);

    template <typename T>
    std::vector<T> construct_set_(const std::vector<bool> &bits, const std::vector<T>& vals);

    bool verbose_;

};


#endif //VERIFYPN_PETRINETSERIALIZER_H
