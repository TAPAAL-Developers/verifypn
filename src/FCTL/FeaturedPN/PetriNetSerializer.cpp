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


#include "FCTL/FeaturedPN/PetriNetSerializer.h"
#include "FCTL/FeaturedPN/FeatureInstantiator.h"
#include "utils/errors.h"

bool logging = false;

struct Assignment {
    explicit Assignment(spot::bdd_dict_ptr dict, const std::vector<spot::formula>& pos_vars)
        : dict_(dict)
    {
        for (const auto& [f, bdd] : dict_->var_map) {
            if (!f.is_literal()) continue;
            assert(!f.is(spot::op::Not));
            if (std::find(pos_vars.begin(), pos_vars.end(), f) != std::end(pos_vars)) {
                // variable is positive
                this->pos_vars &= bdd_ithvar(bdd);
            }
            else {
                this->pos_vars &= bdd_nithvar(bdd);
            }
        }
    }

    bdd pos_vars = bddtrue;
    spot::bdd_dict_ptr dict_;
    bdd neg_vars = bddtrue;
    [[nodiscard]] bool operator()(bdd feature) const {
        if (feature == bddtrue) return true;
        if (logging) {
            std::cerr << "Transition predicate: " << spot::bdd_to_formula(feature, dict_)
                      << "\nPositive vars: " << spot::bdd_to_formula(pos_vars, dict_);
        }
        bdd res = bdd_restrict(feature, pos_vars);
        if (logging) {
            std::cerr << "\nRestricted: " << spot::bdd_to_formula(res, dict_) << ".\n\n";
        }

        assert(res == bddtrue || res == bddfalse);
        return res == bddtrue;
    }
};

PetriNetSerializer::PetriNetSerializer(PetriEngine::PetriNetBuilder& builder, bool verbose) : builder_(builder), verbose_(verbose) {
#ifndef NDEBUG
    logging = verbose_;
#endif
}

void PetriNetSerializer::serialize_features(const std::filesystem::path& fname) {
    auto odir = is_directory(fname) ? fname : fname.parent_path();

    auto feats = builder_.get_features();
    std::vector vec(feats.begin(), feats.end());
    if (feats.size() > 16) {
        throw base_error{"Cowardly refusing to enumerate 2^", feats.size(), " Petri nets."};
    }
    auto feature_sets = enumerate(vec);
    FeatureInstantiator instantiator(builder_);
    auto net = std::unique_ptr<PetriEngine::PetriNet>(builder_.makePetriNet());
    for (int i = 0; i < feature_sets.size(); ++i) {
        if (verbose_) {
            std::cerr << "========================================================\n"
                      << "Generating net " << i+1 << " of " << feature_sets.size() << "\n"
                      << "========================================================\n";
        }
        const auto& feats = feature_sets[i];
        Assignment pred{builder_.bdd_dict, feats};
        std::vector<uint32_t> bad;
        for (uint32_t j = 0; j < net->numberOfTransitions(); ++j) {
            if (!pred(net->feat(j))) {
                bad.emplace_back(j);
            }
        }
        //auto stripped_net = std::unique_ptr<PetriEngine::PetriNet>(instantiator.make_stripped_petri_net(pred));
        //assert(!stripped_net->is_featured());

        std::filesystem::path path = odir / ("featured-enumerate-" + std::to_string(i+1) + ".pnml");
        std::ofstream ofs{path};
        net->toXML(ofs, builder_.bdd_dict, [&bad](uint32_t tid) { return std::find(bad.begin(), bad.end(), tid) == std::end(bad); });

        if (verbose_) {
            std::cerr << std::endl;
        }
    }
}

template <typename T>
std::vector<T> PetriNetSerializer::construct_set_(const std::vector<bool> &bits, const std::vector<T>& vals)
{
    std::vector<T> set;
    for (size_t i = 0; i < bits.size(); ++i) {
        if (bits[i]) {
            set.emplace_back(vals[i]);
        }
    }
    return set;
}

template <typename T>
std::vector<std::vector<T>> PetriNetSerializer::enumerate(const std::vector<T>& vals)
{
    std::vector<std::vector<T>> sets;
    std::vector<bool> curr(vals.size(), false);

    enumerate_rec(vals, sets, curr, 0);
    assert(sets.size() == (1 << vals.size()));
    return sets;
}

template <typename T>
void PetriNetSerializer::enumerate_rec(const std::vector<T>& vals, std::vector<std::vector<T>>& sets, std::vector<bool>& current_set, int depth)
{
    if (depth >= current_set.size()) {
        sets.push_back(construct_set_(current_set, vals));
    }
    else {
        for (auto use : {true, false}) {
            current_set.at(depth) = use;
            enumerate_rec(vals, sets, current_set, depth+1);
        }
    }
}
