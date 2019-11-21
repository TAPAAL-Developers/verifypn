/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                     Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                     Lars Kærlund Østergaard <larsko@gmail.com>,
 *                     Peter Gjøl Jensen <root@petergjoel.dk>
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
#ifndef CONTEXTS_H
#define CONTEXTS_H


#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <chrono>

#include "../PetriNet.h"
#include "../NetStructures.h"
#include "ExprError.h"

namespace PetriEngine {
    namespace Simplification
    {
        class LPCache;
    }
    namespace PQL {

        /** Context provided for context analysis */
        class AnalysisContext {
        protected:
            const std::unordered_map<std::string, uint32_t>& _placeNames;
            const std::unordered_map<std::string, uint32_t>& _transitionNames;
            const PetriNet* _net;
            std::vector<ExprError> _errors;
        public:

            /** A resolution result */
            struct ResolutionResult {
                /** Offset in relevant vector */
                int offset;
                /** True, if the resolution was successful */
                bool success;
            };

            AnalysisContext(const std::unordered_map<std::string, uint32_t>& places, const std::unordered_map<std::string, uint32_t>& tnames, const PetriNet* net)
            : _placeNames(places), _transitionNames(tnames), _net(net) {

            }
            
            virtual void setHasDeadlock(){};
            
            const PetriNet* net() const
            {
                return _net;
            }
            
            /** Resolve an identifier */
            virtual ResolutionResult resolve(const std::string& identifier, bool place = true) {
                ResolutionResult result;
                result.offset = -1;
                result.success = false;
                auto& map = place ? _placeNames : _transitionNames;
                auto it = map.find(identifier);
                if(it != map.end())
                {
                    result.offset = (int)it->second;
                    result.success = true;
                    return result;
                }                
                return result;
            }

            /** Report error */
            void reportError(const ExprError& error) {
                _errors.push_back(error);
            }

            /** Get list of errors */
            const std::vector<ExprError>& errors() const {
                return _errors;
            }
        };

        class ColoredAnalysisContext : public AnalysisContext {
        protected:
            const std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>>& _coloredPlaceNames;
            const std::unordered_map<std::string, std::vector<std::string>>& _coloredTransitionNames;

            bool _colored;

        public:
            ColoredAnalysisContext(const std::unordered_map<std::string, uint32_t>& places,
                                   const std::unordered_map<std::string, uint32_t>& tnames,
                                   const PetriNet* net,
                                   const std::unordered_map<std::string, std::unordered_map<uint32_t , std::string>>& cplaces,
                                   const std::unordered_map<std::string, std::vector<std::string>>& ctnames,
                                   bool colored)
                    : AnalysisContext(places, tnames, net),
                      _coloredPlaceNames(cplaces),
                      _coloredTransitionNames(ctnames),
                      _colored(colored)
            {}

            bool resolvePlace(const std::string& place, std::unordered_map<uint32_t,std::string>& out) {
                auto it = _coloredPlaceNames.find(place);
                if (it != _coloredPlaceNames.end()) {
                    out = it->second;
                    return true;
                }
                return false;
            }

            bool resolveTransition(const std::string& transition, std::vector<std::string>& out) {
                auto it = _coloredTransitionNames.find(transition);
                if (it != _coloredTransitionNames.end()) {
                    out = it->second;
                    return true;
                }
                return false;
            }

            bool isColored() const {
                return _colored;
            }
        };

        /** Context provided for evalation */
        class EvaluationContext {
        public:

            /** Create evaluation context, this doesn't take ownership */
            EvaluationContext(const MarkVal* marking,
                    const PetriNet* net, bool is_game = false)
            : _marking(marking), _net(net), _is_game(is_game) {}
            
            EvaluationContext() = default;

            const MarkVal* marking() const {
                return _marking;
            }
            
            void setMarking(MarkVal* marking) {
                _marking = marking;
            }

            const PetriNet* net() const {
                return _net;
            }
            bool isGame() const {
                return _is_game;
            }

            void setPlaceChange(const std::pair<uint32_t,uint32_t>* stats)
            {
                _place_change = stats;
            }
            
            MarkVal value(uint32_t p) const
            {
                assert(_marking);
                return _marking[p];
            }
            
            MarkVal upper(uint32_t p) const
            {
                return _place_change == nullptr ? 
                        std::numeric_limits<MarkVal>::max() :
                        _place_change[p].second;
            }
            
            MarkVal lower(uint32_t p) const
            {
                return _place_change == nullptr ? 
                        0 :
                        _place_change[p].first;
            }
            
        private:
            const MarkVal* _marking = nullptr;
            const PetriNet* _net = nullptr;
            const bool _is_game = false;
            const std::pair<uint32_t, uint32_t>* _place_change = nullptr;
        };

        /** Context for distance computation */
        class DistanceContext : public EvaluationContext {
        public:

            DistanceContext(const PetriNet* net,
                    const MarkVal* marking)
            : EvaluationContext(marking, net) {
                _negated = false;
            }


            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }

        private:
            bool _negated;
        };

        /** Context for condition to TAPAAL export */
        class TAPAALConditionExportContext {
        public:
            bool failed;
            std::string netName;
        };

        class SimplificationContext {
        public:

            SimplificationContext(const MarkVal* marking,
                    const PetriNet* net, uint32_t queryTimeout, uint32_t lpTimeout,
                    Simplification::LPCache* cache)
                    : _queryTimeout(queryTimeout), _lpTimeout(lpTimeout) {
                _negated = false;
                _marking = marking;
                _net = net;
                _start = std::chrono::high_resolution_clock::now();
                _cache = cache;
            }

            const MarkVal* marking() const {
                return _marking;
            }

            const PetriNet* net() const {
                return _net;
            }

            void negate() {
                _negated = !_negated;
            }

            bool negated() const {
                return _negated;
            }
            
            void setNegate(bool b){
                _negated = b;
            }
            
            double getReductionTime(){
                // duration in seconds
                auto end = std::chrono::high_resolution_clock::now();
                return (std::chrono::duration_cast<std::chrono::microseconds>(end - _start).count())*0.000001;
            }
            
            bool timeout() const {
                auto end = std::chrono::high_resolution_clock::now();
                auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
                return (diff.count() >= _queryTimeout);
            }
            
            uint32_t getLpTimeout() const {
                return _lpTimeout;
            }
            
            Simplification::LPCache* cache() const
            {
                return _cache;
            }

        private:
            bool _negated;
            const MarkVal* _marking;
            const PetriNet* _net;
            uint32_t _queryTimeout, _lpTimeout;
            std::chrono::high_resolution_clock::time_point _start;
            Simplification::LPCache* _cache;
        };

    } // PQL
} // PetriEngine

#endif // CONTEXTS_H
