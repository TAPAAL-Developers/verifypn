/*
 * Authors:
 *      Nicolaj Østerby Jensen
 *      Jesper Adriaan van Diepen
 *      Mathias Mehl Sørensen
 */
#include <PetriEngine/Colored/PartitionBuilder.h>
#include <PetriEngine/Colored/EvaluationVisitor.h>
#include "PetriEngine/Colored/Reduction/RedRulePreemptiveFiring.h"
#include "PetriEngine/Colored/Reduction/ColoredReducer.h"
#include "PetriEngine/Colored/ArcVarMultisetVisitor.h"

namespace PetriEngine::Colored::Reduction {
    bool RedRulePreemptiveFiring::apply(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                        QueryType queryType,
                                        bool preserveLoops, bool preserveStutter) {

        bool continueReductions = false;

        const size_t numberofplaces = red.placeCount();
        for (uint32_t p = 0; p < numberofplaces; ++p) {
            if (red.hasTimedOut()) return false;

            auto &place = const_cast<Place &>(red.places()[p]);
            if (place.skipped) continue;
            if (place.marking.empty()) continue;
            if (place.inhibitor) continue;
            if (inQuery.isPlaceUsed(p)) continue;

            // Must be exactly one post, in order to not remove branching
            if (place._post.size() != 1) {
                continue;
            }

            if (!t_is_viable(red, inQuery, place._post[0], p)) continue;

            const Transition &transition = red.transitions()[place._post[0]];

            fired.insert(transition.name);

            const Multiset tokens = place.marking;

            for (auto &out: transition.output_arcs) {
                auto &otherplace = const_cast<Place &>(red.places()[out.place]);
                otherplace.marking += tokens;
            }

            if (place._pre.empty()) {
                red.skipPlace(p);
            } else {
                place.marking = Multiset();
            }

            _applications++;
            continueReductions = true;

        }
        return continueReductions;
    }

    bool RedRulePreemptiveFiring::t_is_viable(ColoredReducer &red, const PetriEngine::PQL::ColoredUseVisitor &inQuery,
                                              uint32_t t, uint32_t p) {
        //fireability consistency check
        if (inQuery.isTransitionUsed(t)) return false;

        const Transition &transition = red.transitions()[t];

        // Only fire each transition once to avoid infinite loops
        if (fired.find(transition.name) != fired.end()) return false;

        // Easiest to not handle guards
        if (transition.guard) return false;
        if (transition.inhibited) return false;
        if (transition.input_arcs.size() > 1) return false;

        // We must be able to move all the tokens
        auto &place = red.places()[p];
        const auto &in = red.getInArc(p, transition);
        uint32_t inWeight = red.getInArc(p, transition)->expr->weight();
        if (!place._pre.empty() && inWeight != 1) return false;
        if (place.marking.distinctSize() > 1 && inWeight != 1) return false;
        if ((place.marking.size() % inWeight) != 0) return false;

        // Post set cannot inhibit or be in query
        for (auto &out: transition.output_arcs) {
            auto &outPlace = red.places()[out.place];
            if (inQuery.isPlaceUsed(out.place) || outPlace.inhibitor) {
                return false;
            }


            // For fireability consistency. We don't want to put tokens to a place enabling transition
            for (auto &tin: outPlace._post) {
                if (inQuery.isTransitionUsed(tin)) {
                    return false;
                }
            }

            // todo could relax this, and instead of simply copying the tokens to the new place, then update them according to the out arc expression
            // todo or simple extension, check if constant color on the out arc
            if (to_string(*out.expr) != to_string(*in->expr)) {
                return false;
            }
        }

        return true;
    }
}