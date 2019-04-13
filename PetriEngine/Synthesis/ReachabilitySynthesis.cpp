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

#define TRYSYNTH(X,S,Q,P)    if(S) run<ReducingSuccessorGenerator,X>(Q, P); \
                             else  run<SuccessorGenerator,X>(Q, P);

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
                switch (strategy) {
                    case Utils::SearchStrategies::DFS:
                        TRYSYNTH(DFSQueue, use_stubborn, result, permissive)
                        break;
                    case Utils::SearchStrategies::BFS:
                        TRYSYNTH(BFSQueue, use_stubborn, result, permissive)
                        break;
                    /*case Utils::SearchStrategies::HEUR:
                        TRYSYNTH(HeuristicQueue, use_stubborn, result, permissive)
                        break;*/
                    case Utils::SearchStrategies::RDFS:
                        TRYSYNTH(RDFSQueue, use_stubborn, result, permissive)
                        break;
                    default:
                        std::cerr << "Unsopported Search Strategy for Synthesis" << std::endl;
                        exit(ErrorCode);
                }


                printer.printResult(result);
            }
            return SuccessCode;
        }

        size_t ReachabilitySynthesis::dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety) {
            size_t processed = 0;
            for (auto& dep : next->_dependers) {
                ++processed;
                std::cerr << "DEP OF " << next->_marking << " : " << dep.second->_marking << " FROM " << (dep.first ? "CTRL" : "ENV") << std::endl;
                SynthConfig* ancestor = dep.second;
                std::cerr << "AND WAIT " << (int)ancestor->_waiting << std::endl;
                if (ancestor->determined()) 
                    continue;
                std::cerr << "PRE STATE " << (int) ancestor->_state << std::endl;
                std::cerr << "CHILDREN " << ancestor->_ctrl_children << " : " << ancestor->_env_children << std::endl;
                bool ctrl_child = dep.first;
                if (ctrl_child) {
                    ancestor->_ctrl_children -= 1;
                    //std::cerr << "NEXT " << (int)next->_state << std::endl;
                    if (next->_state == SynthConfig::WINNING) {
                        if(ancestor->_env_children == 0)
                            ancestor->_state = SynthConfig::WINNING;
                        else
                            ancestor->_state = SynthConfig::MAYBE;
                    }

                    
                    if (ancestor->_ctrl_children == 0 && // no more tries, and no potential or certainty
                        (ancestor->_state & (SynthConfig::MAYBE | SynthConfig::WINNING)) == 0)
                        ancestor->_state = SynthConfig::LOSING;
                    std::cerr << "CTRL MODE " << ancestor->_ctrl_children << std::endl;
                    std::cerr << "STATE " << (int)ancestor->_state << std::endl;
                } else {
                    ancestor->_env_children -= 1;
                    if (next->_state == SynthConfig::LOSING) 
                        ancestor->_state = SynthConfig::LOSING;
                    else if(next->_state == SynthConfig::WINNING)
                        ancestor->_state = SynthConfig::MAYBE;
                    std::cerr << "ENV MODE " << ancestor->_env_children << std::endl;
                    std::cerr << "STATE " << (int)ancestor->_state << std::endl;
                }

                if (ancestor->_env_children == 0 && ancestor->_ctrl_children == 0 && ancestor->_state == SynthConfig::MAYBE) {
                    ancestor->_state = SynthConfig::WINNING;
                    std::cerr << "UP TO WIN" << std::endl;
                }

                if (ancestor->determined()) {
                    if (ancestor->_waiting < 2) 
                        back.push(ancestor);
                    ancestor->_waiting = 2;
                }
            }
            if(processed == 0)
                std::cerr << "NO DEPS" << std::endl;
            next->_dependers.clear();
            return processed;
        }
        
        bool ReachabilitySynthesis::check_bound(const MarkVal* marking) {
            if (_kbound > 0) {
                size_t sum = 0;
                for (size_t p = 0; p < _net.numberOfPlaces(); ++p)
                    sum += marking[p];
                if (_kbound < sum)
                    return false;
            }
            return true;
        }
        
        bool ReachabilitySynthesis::eval(PQL::Condition* cond, const MarkVal* marking) {
            PQL::EvaluationContext ctx(marking, &_net);
            return cond->evaluate(ctx) == PQL::Condition::RTRUE;
        }
        
        SynthConfig& ReachabilitySynthesis::get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, const MarkVal* marking, PQL::Condition* prop, bool is_safety, size_t& cid) {
            // TODO, we don't actually have to store winning markings here (what is fastest, checking query or looking up marking?/memory)!
            auto res = stateset.add(marking);
            cid = res.second;
            SynthConfig& meta = stateset.get_data(res.second);
            std::cerr << "C " << res.second << std::endl;
            if (res.first) {
                std::cerr << "NEW ";
                _net.print(marking);
                meta = {SynthConfig::UNKNOWN, false, 0, 0, SynthConfig::depends_t(), res.second};
                if (!check_bound(marking))
                {
                    meta._state = SynthConfig::LOSING;
                }
                else {
                    auto res = eval(prop, marking);
                    if (is_safety) {
                        if (res == false)
                        {
                            meta._state = SynthConfig::LOSING;
                            std::cerr << "L" << std::endl;
                        }
                        else
                        {
                            meta._state = SynthConfig::MAYBE;
                            std::cerr << "M" << std::endl;
                        }
                    } else {
                        if (res == false)
                        {
                            meta._state = SynthConfig::MAYBE;
                            std::cerr << "M" << std::endl;
                        }
                        else
                        {
                            meta._state = SynthConfig::WINNING;
                            std::cerr << "W" << std::endl;
                        }
                    }
                }
            }
            return meta;
        }

    }
}