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
            permissive = true;
            using namespace Structures;
            for (size_t qnum = 0; qnum < queries.size(); ++qnum) {
                ResultPrinter::DGResult result(qnum, queries[qnum].get());
                switch (strategy) {
                    case Utils::SearchStrategies::HEUR:
                        std::cout << "Using DFS instead of BestFS for synthesis" << std::endl;
                        /*TRYSYNTH(HeuristicQueue, use_stubborn, result, permissive)
                        break;*/
                    case Utils::SearchStrategies::DFS:
                        TRYSYNTH(DFSQueue, use_stubborn, result, permissive)
                        break;
                    case Utils::SearchStrategies::BFS:
                        TRYSYNTH(BFSQueue, use_stubborn, result, permissive)
                        break;                    
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

                SynthConfig* ancestor = dep.second;
                if (ancestor->determined()) 
                    continue;
                bool ctrl_child = dep.first;
                if (ctrl_child) {
                    ancestor->_ctrl_children -= 1;
                    if (next->_state == SynthConfig::WINNING) {
                        if(ancestor->_env_children == 0)
                            ancestor->_state = SynthConfig::WINNING;
                        else
                            ancestor->_state = SynthConfig::MAYBE;
                    }

                    
                    if (ancestor->_ctrl_children == 0 && // no more tries, and no potential or certainty
                        (ancestor->_state & (SynthConfig::MAYBE | SynthConfig::WINNING)) == 0)
                        ancestor->_state = SynthConfig::LOSING;
                } else {
                    ancestor->_env_children -= 1;
                    if (next->_state == SynthConfig::LOSING) 
                        ancestor->_state = SynthConfig::LOSING;
                    else if(next->_state == SynthConfig::WINNING)
                    {
                        if(ancestor->_env_children == 0 && ancestor->_state == SynthConfig::MAYBE)
                            ancestor->_state = SynthConfig::WINNING;
                    }
                }

                if (ancestor->_env_children == 0 && ancestor->_ctrl_children == 0 && ancestor->_state == SynthConfig::MAYBE) {
                    ancestor->_state = SynthConfig::WINNING;
                }

                if (ancestor->determined()) {
                    if (ancestor->_waiting < 2) 
                        back.push(ancestor);
                    ancestor->_waiting = 2;
                }
            }
            //if(processed == 0)
            //    std::cerr << "NO DEPS" << std::endl;
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
            return cond->evaluate(ctx).first == PQL::RTRUE; 
            // TODO, we can use the stability in the fixpoint computation to prun the Dep-graph
        }
        
#ifndef NDEBUG
        std::vector<MarkVal*> markings;
#endif
        
        SynthConfig& ReachabilitySynthesis::get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, const MarkVal* marking, PQL::Condition* prop, bool is_safety, size_t& cid) {
            // TODO, we don't actually have to store winning markings here (what is fastest, checking query or looking up marking?/memory)!
            auto res = stateset.add(marking);
            cid = res.second;
            SynthConfig& meta = stateset.get_data(res.second);
            {
#ifndef NDEBUG
                MarkVal* tmp = new MarkVal[_net.numberOfPlaces()];
                stateset.decode(tmp, res.second);
                std::memcmp(tmp, marking, sizeof(MarkVal)*_net.numberOfPlaces());
                delete[] tmp;
#endif
            }
            //std::cerr << "C " << res.second << std::endl;
            if (res.first) {

#ifndef NDEBUG
                markings.push_back(new MarkVal[_net.numberOfPlaces()]);
                memcpy(markings.back(), marking, sizeof(MarkVal)*_net.numberOfPlaces());
#endif
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
                            //std::cerr << "L" << std::endl;
                        }
                        else
                        {
                            meta._state = SynthConfig::MAYBE;
                            //std::cerr << "M" << std::endl;
                        }
                    } else {
                        if (res == false)
                        {
                            meta._state = SynthConfig::MAYBE;
                            //std::cerr << "M" << std::endl;
                        }
                        else
                        {
                            meta._state = SynthConfig::WINNING;
                            //std::cerr << "W" << std::endl;
                        }
                    }
                }
            }
            
            
            return meta;
        }

#ifndef NDEBUG
        void ReachabilitySynthesis::print_id(size_t id)
        {
            _net.print(markings[id]); 
        }


        // validating the solution of the DEP graph (reachability-query is assumed)
        void ReachabilitySynthesis::validate(PQL::Condition* query, Structures::AnnotatedStateSet<SynthConfig>& stateset)
        {
            MarkVal* working = new MarkVal[_net.numberOfPlaces()];
            size_t old = markings.size();
            for(size_t id = 0; id < old; ++id)
            {
                std::cerr << "VALIDATION " << id << std::endl;
                auto& conf = stateset.get_data(id);
                if(conf._state == SynthConfig::UNKNOWN) 
                    continue;
                PQL::EvaluationContext ctx(markings[id], &_net);
                auto res = query->evaluate(ctx);
                if(conf._state != SynthConfig::WINNING)
                    assert(res.first != RTRUE);
                else
                {
                    continue;
                }
                SuccessorGenerator generator(_net, true, false);
                generator.prepare(markings[id]);
                bool ok = false;
                std::vector<size_t> env_maybe;
                while (generator.next(working, PetriNet::ENV)) {
                    auto res = stateset.add(working);
                    assert(!res.first);
                    if(!res.first)
                    {
                        auto& c = stateset.get_data(res.second);
                        if(c._state == SynthConfig::LOSING)
                        {
                            assert(conf._state == SynthConfig::LOSING);
                            std::cerr << "[" << id << "] -E-> [" << res.second << "]\n";
                            ok = true;
                            break;
                        }
                        else if(c._state != SynthConfig::WINNING)
                        {
                            env_maybe.push_back(res.second);
                        }
                    }
                }
                if(!ok)
                {
                    if(env_maybe.size() > 0)
                    {
                        assert(conf._state != SynthConfig::WINNING);
                        for(auto i : env_maybe)
                        {
                                std::cerr << "[" << id << "] -E-> [" << i << "]\n";
                        }
                        continue;
                    }
                    std::vector<size_t> not_win;
                    while (generator.next(working, PetriNet::CTRL)) {
                        auto res = stateset.add(working);
                        assert(!res.first);
                        if(!res.first)
                        {
                            auto& c = stateset.get_data(res.second);
                            if(c._state == SynthConfig::WINNING)
                            {
                                std::cerr << "[" << id << "] -C-> [" << res.second << "]\n";
                                std::cerr << (int)c._state << " :: " << c._marking << std::endl;
                                std::cerr << "ANCESTOR " << (int) conf._state << " :: " << conf._marking << std::endl;

                                assert(conf._state == SynthConfig::WINNING);
                                ok = true;
                                break;
                            }
                            else
                            {
                                not_win.push_back(res.second);
                            }
                        }
                    }
                    assert(ok || conf._state != SynthConfig::WINNING);
                    if(!ok)
                    {
                        for(auto i : not_win)
                        {
                            std::cerr << "[" << id << "] -C-> [" << i << "]\n";
                        }
                    }
                }
            }
            delete[] working;
        }
#endif
        template<>
        void ReachabilitySynthesis::setQuery<ReducingSuccessorGenerator>(ReducingSuccessorGenerator& generator, PQL::Condition* query, bool is_safety)
        {
            generator.setQuery(query, is_safety);
        }
    }
}