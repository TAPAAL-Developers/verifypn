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
#include "Utils/Stopwatch.h"

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
                    bool permissive = false,
                    std::ostream* strategy_out = nullptr);
        private:

            
            bool eval(PQL::Condition* cond, const MarkVal* marking);

            bool check_bound(const MarkVal* marking);

            size_t dependers_to_waiting(SynthConfig* next, std::stack<SynthConfig*>& back, bool safety);
            
            template<typename GENERATOR>
            void setQuery(GENERATOR&, PQL::Condition* query, bool is_safety)
            {                
            }
            
            void print_strategy(std::ostream& strategy_out, Structures::AnnotatedStateSet<SynthConfig>& stateset, SynthConfig& meta, bool is_safety);

#ifndef NDEBUG
            void print_id(size_t);
            void validate(PQL::Condition*, Structures::AnnotatedStateSet<SynthConfig>&, bool is_safety);
#endif
            
            template <typename GENERATOR, typename QUEUE>
            void run(ResultPrinter::DGResult& result, bool permissive, std::ostream* strategy_out) {
                // permissive == maximal in this case; there is a subtle difference
                // in wether you terminate the search at losing states (permissive)
                // or you saturate over the entire graph (maximal)
                // the later includes potential 
                // safety/reachability given "wrong choices" on both sides
                auto query = const_cast<PQL::Condition*>(result.query);
                const bool is_safety = query->isInvariant();
                if (query == nullptr) {
                    std::cerr << "Body of quantifier is null" << std::endl;
                    std::exit(ErrorCode);
                }
                if (query->isTemporal()) {
                    std::cerr << "Body of quantifier is temporal" << std::endl;
                    std::exit(ErrorCode);
                }
                /*if(is_safety)
                {
                    std::cerr << "Safety synthesis is untested and unsupported" << std::endl;
                    std::exit(ErrorCode);
                }*/
                stopwatch timer;
                timer.start();
                auto working = _net.makeInitialMarking();
                auto parent = _net.makeInitialMarking();

                Structures::AnnotatedStateSet<SynthConfig> stateset(_net, 0);
                
                size_t cid;
                size_t nid;

                auto& meta = get_config(stateset, working.get(), query, is_safety, cid);
                meta._waiting = 1;

                QUEUE queue;
                GENERATOR generator(_net, true, is_safety);
                setQuery<GENERATOR>(generator, query, is_safety);
                std::stack<SynthConfig*> back;
                queue.push(cid, nullptr, nullptr);
                
                // these could be preallocated; at most one successor pr transition
                std::vector<std::pair<size_t, SynthConfig*>> env_buffer;
                std::vector<std::pair<size_t, SynthConfig*>> ctrl_buffer;
#ifndef NDEBUG
restart:
#endif
                while (!meta.determined() || permissive) {
                    while (!back.empty()) {
                        SynthConfig* next = back.top();
                        back.pop();
                        result.processedEdges += dependers_to_waiting(next, back, is_safety);
                    }

                    bool any = queue.pop(nid);
                    if (!any) 
                        break;
                    auto& cconf = stateset.get_data(nid);
                    if (cconf.determined())
                    {
                        if(permissive && !cconf._dependers.empty())
                            back.push(&cconf);
                        continue; // handled already
                    }
                    // check predecessors
                    bool any_undet = false;
                    for(auto& p : cconf._dependers)
                    {
                        SynthConfig* sc = p.second;
                        if(sc->determined()) continue;
                        //if(sc->_state == SynthConfig::MAYBE && !p.first && !permissive)
                        //    continue;
                        any_undet = true;
                        break;
                    }
                    if(!any_undet && &cconf != &meta)
                    {
                        cconf._waiting = false;
                        cconf._dependers.clear();
                        continue;
                    }
                    
                    //std::cerr << "PROCESSING [" << cconf._marking << "]" << std::endl;
                    ++result.exploredConfigurations;
                    env_buffer.clear();
                    ctrl_buffer.clear();
                    
                    assert(cconf._waiting == 1);
                    cconf._state = SynthConfig::PROCESSED;
                    assert(cconf._marking == nid);
                    stateset.decode(parent.get(), nid);
                    generator.prepare(parent.get());
                    // first try all environment choices (if one is losing, everything is lost)
                    bool some_env = false;
                    while (generator.next(working.get(), PetriNet::ENV)) {
                        some_env = true;
                        auto& child = get_config(stateset, working.get(), query, is_safety, cid);
                        //std::cerr << "ENV[" << cconf._marking << "] --> [" << child._marking << "]" << std::endl;
                        if (child._state == SynthConfig::LOSING)
                        {
                            // Environment can force a lose
                            cconf._state = SynthConfig::LOSING;
                            break;
                        }
                        else if (child._state == SynthConfig::WINNING)
                            continue; // would never choose 
                        if(&child == &cconf)
                        {
                            if(is_safety) continue; // would make ctrl win
                            else 
                            {
                                cconf._state = SynthConfig::LOSING; // env wins surely
                                break;
                            }
                        }
                        env_buffer.emplace_back(cid, &child);
                    }
                    
                    // if determined, no need to add more, just backprop (later)
                    bool some = false;
                    bool some_winning = false;
                    //tsd::cerr << "CTRL " << std::endl;
                    if(!cconf.determined())
                    {
                        while (generator.next(working.get(), PetriNet::CTRL)) {
                            some = true;
                            auto& child = get_config(stateset, working.get(), query, is_safety, cid);
                            //std::cerr << "CTRL[" << cconf._marking << "] --> [" << child._marking << "]" << std::endl;

                            if(&child == &cconf)
                            {
                                if(is_safety)
                                {   // maybe a win if safety ( no need to explore more)
                                    cconf._state = SynthConfig::MAYBE;
                                    if(!permissive)
                                    {
                                        ctrl_buffer.clear();
                                        if(env_buffer.empty())
                                        {
                                            assert(cconf._state != SynthConfig::LOSING);
                                            cconf._state = SynthConfig::WINNING;
                                        }
                                        break;
                                    }
                                }
                                else
                                {   // would never choose 
                                    continue;
                                }
                            }
                            else if (child._state == SynthConfig::LOSING)
                            {
                                continue; // would never choose
                            }
                            else if (child._state == SynthConfig::WINNING)
                            {
                                some_winning = true;
                                // no need to search further! We are winning if env. cannot force us away
                                cconf._state = SynthConfig::MAYBE;
                                if(!permissive)
                                {
                                    ctrl_buffer.clear();
                                    if(env_buffer.empty())
                                    {
                                        assert(cconf._state != SynthConfig::LOSING);
                                        cconf._state = SynthConfig::WINNING;        
                                    }
                                    break;
                                }
                            }
                            ctrl_buffer.emplace_back(cid, &child);
                        }                        
                    }
                    if(!cconf.determined())
                    {
                        if(some && !some_winning && ctrl_buffer.empty()) // we had a choice but all of them were bad. Env. can force us.
                        {
                            assert(cconf._state != SynthConfig::WINNING);
                            cconf._state = SynthConfig::LOSING;
                        }
                        else if(!some && !some_env) 
                        {
                            // deadlock, bad if reachability, good if safety
                            assert(cconf._state != SynthConfig::WINNING);
                            if(is_safety)
                            {
                                cconf._state = SynthConfig::WINNING;
                            }
                            else
                            {
                                cconf._state = SynthConfig::LOSING;                                   
                            }
                        }
                        else if(env_buffer.empty() && some_winning) 
                        {
                            // reachability: env had not bad choice and ctrl had winning
                            assert(cconf._state != SynthConfig::LOSING);
                            cconf._state = SynthConfig::WINNING;
                        }
                        else if(!some && some_env && env_buffer.empty())
                        {
                            // env is forced to be good.
                            cconf._state = SynthConfig::WINNING;
                        }
                        else if(!some && !env_buffer.empty())
                        {
                            cconf._state = SynthConfig::MAYBE;
                        }
                    }
                    // if determined, no need to add to queue, just backprop
                    //std::cerr << "FINISHED " << cconf._marking << " STATE " << (int)cconf._state << std::endl;
                    if(cconf.determined())
                    {
                        //std::cerr << "DET [" << cconf._marking << "] : " << (int)cconf._state << std::endl;
                        back.push(&cconf);
                    }
                    if(!cconf.determined() || permissive)
                    {

                        // we want to explore the ctrl last (assuming DFS). (TODO: check if a queue-split would be good?)
                        //std::cerr << "[" << nid << "]" << std::endl;
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
                        result.numberOfEdges += cconf._ctrl_children + cconf._env_children;
                    }
                }
#ifndef NDEBUG
                /*permissive = true;
                for(size_t i = 0; i < stateset.size(); ++i)
                {
                    auto& c = stateset.get_data(i);
                    if(c.determined())
                    {
                        if(c._waiting == 0)
                        {
                            back.push(&c);
                            c._waiting = 1;
                        }
                    }
                }
                if(back.size() > 0)
                    goto restart;*/
#endif
                assert(!permissive || queue.empty());
                result.numberOfConfigurations = stateset.size();
                result.numberOfMarkings = stateset.size();
                timer.stop();
                result.duration = timer.duration();
                bool res;
                if(!is_safety) res = meta._state == SynthConfig::WINNING;
                else           res = meta._state != meta.LOSING;
                result.result = (res != is_safety) ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
#ifndef NDEBUG
                {
                    // can only check complete solution to dep graph.
                    validate(query, stateset, is_safety);
                }
#endif
                
                if(strategy_out != nullptr && res) {
                    print_strategy(*strategy_out, stateset, meta, is_safety);
                }
            }

            SynthConfig& get_config(Structures::AnnotatedStateSet<SynthConfig>& stateset, const MarkVal* marking, PQL::Condition* prop, bool is_safety, size_t& cid);

            size_t _kbound;
            PetriNet& _net;
        };
    }
}

#endif /* REACHABILITYSYNTHESIS_H */

