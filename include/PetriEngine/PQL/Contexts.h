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

#include "../NetStructures.h"
#include "../PetriNet.h"
#include "PQL.h"

#include <chrono>
#include <glpk.h>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace PetriEngine {
namespace PQL {

/** Context provided for context analysis */
class AnalysisContext {
  protected:
    const std::unordered_map<std::string, uint32_t> &_placeNames;
    const std::unordered_map<std::string, uint32_t> &_transitionNames;
    const PetriNet *_net;
    std::vector<ExprError> _errors;

  public:
    /** A resolution result */
    struct ResolutionResult {
        /** Offset in relevant vector */
        int _offset;
        /** True, if the resolution was successful */
        bool _success;
    };

    AnalysisContext(const std::unordered_map<std::string, uint32_t> &places,
                    const std::unordered_map<std::string, uint32_t> &tnames, const PetriNet *net)
        : _placeNames(places), _transitionNames(tnames), _net(net) {}

    virtual void set_has_deadlock(){};

    const PetriNet &net() const { return *_net; }

    /** Resolve an identifier */
    virtual ResolutionResult resolve(const std::string &identifier, bool place = true);

    /** Report error */
    void report_error(const ExprError &error) { _errors.push_back(error); }

    /** Get list of errors */
    const std::vector<ExprError> &errors() const { return _errors; }
    auto &all_place_names() const { return _placeNames; }
    auto &all_transition_names() const { return _transitionNames; }
};

class ColoredAnalysisContext : public AnalysisContext {
  protected:
    const std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>>
        &_coloredPlaceNames;
    const std::unordered_map<std::string, std::vector<std::string>> &_coloredTransitionNames;

    bool _colored;

  public:
    ColoredAnalysisContext(
        const std::unordered_map<std::string, uint32_t> &places,
        const std::unordered_map<std::string, uint32_t> &tnames, const PetriNet &net,
        const std::unordered_map<std::string, std::unordered_map<uint32_t, std::string>> &cplaces,
        const std::unordered_map<std::string, std::vector<std::string>> &ctnames, bool colored)
        : AnalysisContext(places, tnames, &net), _coloredPlaceNames(cplaces),
          _coloredTransitionNames(ctnames), _colored(colored) {}

    bool resolve_place(const std::string &place, std::unordered_map<uint32_t, std::string> &out);

    bool resolve_transition(const std::string &transition, std::vector<std::string> &out);

    bool is_colored() const { return _colored; }

    auto &all_colored_place_names() const { return _coloredPlaceNames; }
    auto &all_colored_transition_names() const { return _coloredTransitionNames; }
};

/** Context provided for evalation */
class EvaluationContext {
  public:
    /** Create evaluation context, this doesn't take ownership */
    EvaluationContext(const MarkVal *marking, const PetriNet *net) : _marking(marking), _net(net) {}

    EvaluationContext(const MarkVal *marking, const PetriNet &net)
        : _marking(marking), _net(&net) {}

    EvaluationContext(){};

    const MarkVal *marking() const { return _marking; }

    void set_marking(MarkVal *marking) { _marking = marking; }

    const PetriNet *net() const { return _net; }

  private:
    const MarkVal *_marking = nullptr;
    const PetriNet *_net;
};

/** Context for distance computation */
class DistanceContext : public EvaluationContext {
  public:
    DistanceContext(const PetriNet &net, const MarkVal *marking)
        : EvaluationContext(marking, &net) {
        _negated = false;
    }

    void negate() { _negated = !_negated; }

    bool negated() const { return _negated; }

  private:
    bool _negated;
};

/** Context for condition to TAPAAL export */
class TAPAALConditionExportContext {
  public:
    bool _failed;
    std::string _netName;
};

class SimplificationContext {
  public:
    SimplificationContext(const MarkVal *marking, const PetriNet &net, uint32_t queryTimeout,
                          uint32_t lpTimeout)
        : _negated(false), _marking(marking), _net(net), _queryTimeout(queryTimeout),
          _lpTimeout(lpTimeout) {
        _base_lp = build_base();
        _start = std::chrono::high_resolution_clock::now();
    }

    virtual ~SimplificationContext() {
        if (_base_lp != nullptr)
            glp_delete_prob(_base_lp);
        _base_lp = nullptr;
    }

    const MarkVal *marking() const { return _marking; }

    const PetriNet &net() const { return _net; }

    void negate() { _negated = !_negated; }

    bool negated() const { return _negated; }

    void set_negate(bool b) { _negated = b; }

    double get_reduction_time();

    bool timeout() const {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
        return (diff.count() >= _queryTimeout);
    }

    uint32_t get_lp_timeout() const;

    glp_prob *make_base_lp() const;

  private:
    bool _negated;
    const MarkVal *_marking;
    const PetriNet &_net;
    uint32_t _queryTimeout, _lpTimeout;
    std::chrono::high_resolution_clock::time_point _start;
    mutable glp_prob *_base_lp = nullptr;

    glp_prob *build_base() const;
};

} // namespace PQL
} // namespace PetriEngine

#endif // CONTEXTS_H
