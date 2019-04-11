/*
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  Copyright (C) 2019 Peter G. Jensen <root@petergjoel.dk>
 */

/* 
 * File:   ReachabilitySynthesis.cpp
 * Author: Peter G. Jensen <root@petergjoel.dk>
 * 
 * Created on April 8, 2019, 2:19 PM
 */

#include "ReachabilitySynthesis.h"
#include "SynthConfig.h"
#include "Utils/Stopwatch.h"

#include <vector>

using namespace PetriEngine::PQL;
using namespace PetriEngine;

namespace PetriEngine {
    namespace Synthesis {

        ReachabilitySynthesis::ReachabilitySynthesis(ResultPrinter& printer, PetriNet& net, size_t kbound)
        : printer(printer), _net(net), _kbound(kbound) {
        }

        ReachabilitySynthesis::~ReachabilitySynthesis() {
        }

#define TRYSYNTH(X,S,Q,P)    if(S) solved = run<ReducingSuccessorGenerator,X>(Q, P); \
                             else solved = run<SuccessorGenerator,X>(Q, P);

        ReturnValue ReachabilitySynthesis::synthesize(
                std::vector<std::shared_ptr<PQL::Condition> >& queries,
                std::vector<ResultPrinter::Result>& results,
                Utils::SearchStrategies::Strategy strategy,
                bool use_stubborn,
                bool keep_strategies,
                bool permissive) {
            using namespace Structures;
            for (size_t qnum = 0; qnum < queries.size(); ++qnum) {
                ResultPrinter::DGResult result(qnum, queries[qnum].get());
                bool solved;
                stopwatch timer;
                timer.start();
                switch (strategy) {
                    case Utils::SearchStrategies::DFS:
                        TRYSYNTH(DFSQueue, use_stubborn, queries[qnum].get(), permissive)
                        break;
                    case Utils::SearchStrategies::BFS:
                        TRYSYNTH(BFSQueue, use_stubborn, queries[qnum].get(), permissive)
                        break;
                    /*case Utils::SearchStrategies::HEUR:
                        TRYSYNTH(HeuristicQueue, use_stubborn, queries[qnum].get(), permissive)
                        break;*/
                    case Utils::SearchStrategies::RDFS:
                        TRYSYNTH(RDFSQueue, use_stubborn, queries[qnum].get(), permissive)
                        break;
                    default:
                        std::cerr << "Unsopported Search Strategy for Synthesis" << std::endl;
                        exit(ErrorCode);
                }
                timer.stop();
                result.duration = timer.duration();
                result.result = solved ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;

                printer.printResult(result);
            }
            return SuccessCode;
        }

        void ReachabilitySynthesis::dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety) {
            for (auto& dep : next->_dependers) {
                SynthConfig* ancestor = dep.second;
                if (ancestor->determined()) 
                    continue;

                bool ctrl_child = dep.first;
                if (ctrl_child) {
                    ancestor->_ctrl_children -= 1;
                    if (next->_state == SynthConfig::WINNING) {
                        ancestor->_state = SynthConfig::MAYBE;
                    }

                    if (ancestor->_ctrl_children == 0 && ancestor->_state != SynthConfig::MAYBE)
                        ancestor->_state = SynthConfig::LOSING;

                } else {
                    ancestor->_env_children -= 1;
                    if (next->_state == SynthConfig::LOSING) 
                        ancestor->_state = SynthConfig::LOSING;
                }

                if (ancestor->_env_children == 0 && ancestor->_state == SynthConfig::MAYBE) {
                    ancestor->_state = SynthConfig::WINNING;
                }

                if (ancestor->determined()) {
                    if (ancestor->_waiting >= 2) 
                        back.push(ancestor);
                    ancestor->_waiting = 2;
                }
            }
            next->_dependers.clear();
        }

    }
}