/* Copyright (C) 2022  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
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


#ifndef VERIFYPN_CZCYCLEDETECTFPA_H
#define VERIFYPN_CZCYCLEDETECTFPA_H

#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/DependencyGraph/Edge.h"
#include "CTL/DependencyGraph/Configuration.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "CTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Structures/SuccessorQueue.h"

namespace Algorithm {
    class CZCycleDetectFPA : public FixedPointAlgorithm {
    public:
        CZCycleDetectFPA(ReachabilityStrategy type) : FixedPointAlgorithm(type) {}

        bool search(DependencyGraph::BasicDependencyGraph &graph) override;

        ~CZCycleDetectFPA() override {

        }

    private:

        struct stack_entry_t {
            DependencyGraph::Configuration *_config;
            std::vector<DependencyGraph::Edge*> _edges;
            const DependencyGraph::Edge* pop_edge() {
                if(_edges.empty())
                    return nullptr;
                auto* e = _edges.back();
                if(std::all_of(e->targets->assignment.begin(), e->targets->assignment.end(),
                        [](auto* t){ return t->assignment != UNKNOWN; }))
                    _edges.pop_back();
                return e;
            }
        };

        DependencyGraph::BasicDependencyGraph *graph;
        DependencyGraph::Configuration *root;

        void push_edge(DependencyGraph::Configuration *conf);

        std::pair<DependencyGraph::Configuration *, DependencyGraph::Assignment> eval_edge(DependencyGraph::Edge *e);

        void backprop(DependencyGraph::Configuration *c);

        void assign_value(DependencyGraph::Configuration *c, DependencyGraph::Assignment a);

        std::vector<stack_entry_t> _dstack;

        template <typename GoodPred, typename BadPred>
        bool dependent_search(const DependencyGraph::Configuration* c, GoodPred&& pred, BadPred&& bad) const;
    };
}

#endif //VERIFYPN_CZCYCLEDETECTFPA_H
