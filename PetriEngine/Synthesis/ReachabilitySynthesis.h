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
 * File:   ReachabilitySynthesis.h
 * Author: Peter G. Jensen <root@petergjoel.dk>
 *
 * Created on April 8, 2019, 2:19 PM
 */

#include "SynthConfig.h"

#include "PetriEngine/Structures/Queue.h"
#include "PetriEngine/ResultPrinter.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/Expressions.h"

#include "Utils/DependencyGraph/AlgorithmTypes.h"
#include "Utils/SearhStrategies.h"

#include <vector>
#include <memory>
#include <inttypes.h>


#ifndef REACHABILITYSYNTHESIS_H
#define REACHABILITYSYNTHESIS_H

namespace PetriEngine {
    namespace Synthesis {

        class ReachabilitySynthesis {
        private:
            ResultPrinter& printer;

        public:

            ReachabilitySynthesis(ResultPrinter& printer, PetriNet& net, size_t kbound = 0);

            ~ReachabilitySynthesis();

            ReturnValue synthesize(
                    std::vector<std::shared_ptr<PQL::Condition > >& queries,
                    std::vector<ResultPrinter::Result>& results,
                    Utils::SearchStrategies::Strategy strategy,
                    bool use_stubborn = false,
                    bool keep_strategies = false,
                    bool permissive = false);
        private:

            bool eval(PQL::Condition* cond, const MarkVal* marking);

            bool check_bound(const MarkVal* marking);

            void dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety);

            template <typename GENERATOR, typename QUEUE>
            bool run(PQL::Condition* query, bool permissive) {
                const bool is_safety = query->isInvariant();
                if (query == nullptr) {
                    std::cerr << "Body of quantifier is null" << std::endl;
                    exit(ErrorCode);
                }
                if (query->isTemporal()) {
                    std::cerr << "Body of quantifier is temporal" << std::endl;
                    exit(ErrorCode);
                }
                if(is_safety)
                    std::cerr << "SAFETY " << std::endl;

                auto working = _net.makeInitialMarking();
                auto parent = _net.makeInitialMarking();

                Structures::AnnotatedStateSet<SynthConfig> stateset(_net, 0);
                
                size_t cid;
                size_t nid;
                
                auto& meta = get_config(stateset, working.get(), query, is_safety, cid);
                meta._waiting = 1;

                QUEUE queue;
                GENERATOR generator(_net, true, is_safety);
                std::stack<SynthConfig*> back;
                queue.push(cid, nullptr, nullptr);
                
                // these could be preallocated; at most one successor pr transition
                std::vector<std::pair<size_t, SynthConfig*>> env_buffer;
                std::vector<std::pair<size_t, SynthConfig*>> ctrl_buffer;
                
                while (!meta.determined() || permissive) {
                    while (!back.empty()) {
                        SynthConfig* next = back.top();
                        back.pop();
                        std::cerr << "BACK " << next->_marking << std::endl;
                        dependers_to_waiting(next, back, is_safety);
                    }

                    bool any = queue.pop(nid);
                    if (!any) break;
                    auto& cconf = stateset.get_data(nid);
                    if (cconf.determined())
                        continue; // handled already

                    env_buffer.clear();
                    ctrl_buffer.clear();
                    
                    assert(cconf._waiting == 1);
                    cconf._state = SynthConfig::PROCESSED;
                    stateset.decode(parent.get(), nid);
                    generator.prepare(parent.get());
                    // first try all environment choices (if one is losing, everything is lost)
                    std::cerr << "NEXT " << cconf._marking << std::endl;
                    std::cerr << "ENV " << std::endl;
                    bool some_env = false;;
                    while (generator.next(working.get(), PetriNet::ENV)) {
                        some_env = true;
                        auto& child = get_config(stateset, working.get(), query, is_safety, cid);
                        if (child._state == SynthConfig::LOSING)
                        {
                            // Environment can force a lose
                            std::cerr << "CHILD LOSE " << std::endl;
                            cconf._state = SynthConfig::LOSING;
                            break;
                        }
                        else if (child._state == SynthConfig::WINNING)
                        {
                            cconf._state = SynthConfig::MAYBE;
                            continue; // would never choose 
                        }
                        if(&child == &cconf)
                        {
                            if(is_safety) continue; // would make ctrl win
                            else 
                            {
                                std::cerr << "SELFLOOP" << std::endl;
                                cconf._state = SynthConfig::LOSING; // env wins surely
                                break;                                
                            }
                        }
                        env_buffer.emplace_back(cid, &child);
                    }
                    
                    // if determined, no need to add more, just backprop (later)
                    bool some = false;
                    if(!cconf.determined())
                    {
                        std::cerr << "CTRL " << std::endl;
                        while (generator.next(working.get(), PetriNet::CTRL)) {
                            some = true;
                            auto& child = get_config(stateset, working.get(), query, is_safety, cid);
                            if(&child == &cconf)
                            {
                                if(is_safety)
                                {   // maybe a win if safety ( no need to explore more)
                                    cconf._state = SynthConfig::MAYBE;
                                    if(!permissive)
                                    {
                                        ctrl_buffer.clear();
                                        ctrl_buffer.emplace_back(cid, &child);
                                    }
                                    break;
                                }
                                else
                                {   // would never choose 
                                    continue;
                                }
                            }
                            
                            if (child._state == SynthConfig::LOSING)
                                continue; // would never choose
                            else if (child._state == SynthConfig::WINNING)
                            {
                                std::cerr << "WIN CHILD " << std::endl;
                                if(env_buffer.size() == 0) // env does not want to choose, so we can
                                {
                                    cconf._state = SynthConfig::WINNING;
                                    break;
                                }
                                else
                                {
                                    // no need to search further! We are winning if env. cannot force us away
                                    cconf._state = SynthConfig::MAYBE;
                                    if(!permissive)
                                    {
                                        ctrl_buffer.clear();
                                        ctrl_buffer.emplace_back(cid, &child);
                                    }
                                    break;
                                }
                            }
                            ctrl_buffer.emplace_back(cid, &child);
                        }                        
                    }
                    if(some && ctrl_buffer.empty()) // we had a choice but all of them were bad. Env. can force us.
                        cconf._state = SynthConfig::LOSING;
                    else if(!some && (is_safety || some_env) && env_buffer.empty()) // only Env. actions. safety+deadlock = win, or all win of env = win
                        cconf._state = SynthConfig::WINNING;
                    else if(!is_safety && !some && !some_env) // deadlock, bad if reachability
                        cconf._state = SynthConfig::LOSING;
                    // if determined, no need to add to queue, just backprop
                    std::cerr << "FINISHED " << cconf._marking << " STATE " << (int)cconf._state << std::endl;
                    if(cconf.determined())
                    {
                        std::cerr << "REDO " << cconf._marking << std::endl;
                        back.push(&cconf);
                    }
                    else
                    {
                        // we want to explore the ctrl last (assuming DFS). (TODO: check if a queue-split would be good?)
                        for(auto& c : ctrl_buffer)
                        {
                            if(c.second->_waiting == 0)
                            {
                                queue.push(c.first, nullptr, nullptr);
                                c.second->_waiting = 1;
                            }
                            c.second->_dependers.emplace_front(true, &cconf);
                        }
                        // then env.
                        for(auto& c : env_buffer)
                        {
                            if(c.second->_waiting == 0)
                            {
                                queue.push(c.first, nullptr, nullptr);
                                c.second->_waiting = 1;
                            }
                            c.second->_dependers.emplace_front(false, &cconf);
                        }
                        cconf._ctrl_children = ctrl_buffer.size();
                        cconf._env_children = env_buffer.size();
                    }
                }
                if(!is_safety) return meta._state == SynthConfig::WINNING;
                else           return meta._state != meta.LOSING;
            }

            SynthConfig& get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, const MarkVal* marking, PQL::Condition* prop, bool is_safety, size_t& cid);

            size_t _kbound;
            PetriNet& _net;
        };
    }
}

#endif /* REACHABILITYSYNTHESIS_H */

