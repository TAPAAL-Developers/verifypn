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

namespace PetriEngine::PQL {

/** Context provided for context analysis */
class AnalysisContext {
  protected:
    const std::unordered_map<std::string, uint32_t> &_placeNames;
    const std::unordered_map<std::string, uint32_t> &_transitionNames;
    const PetriNet *_net;
    std::vector<ExprError> _errors;

  public:
    /** A resolution result */
    struct resolution_result_t {
        /** Offset in relevant vector */
        int _offset;
        /** True, if the resolution was successful */
        bool _success;
    };

    AnalysisContext(const std::unordered_map<std::string, uint32_t> &places,
                    const std::unordered_map<std::string, uint32_t> &tnames, const PetriNet *net)
        : _placeNames(places), _transitionNames(tnames), _net(net) {}

    virtual void set_has_deadlock(){};

    [[nodiscard]] auto net() const -> const PetriNet & { return *_net; }

    /** Resolve an identifier */
    virtual auto resolve(const std::string &identifier, bool place = true) -> resolution_result_t;

    /** Report error */
    void report_error(const ExprError &error) { _errors.push_back(error); }

    /** Get list of errors */
    [[nodiscard]] auto errors() const -> const std::vector<ExprError> & { return _errors; }
    [[nodiscard]] auto all_place_names() const -> auto & { return _placeNames; }
    [[nodiscard]] auto all_transition_names() const -> auto & { return _transitionNames; }
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

    auto resolve_place(const std::string &place, std::unordered_map<uint32_t, std::string> &out)
        -> bool;

    auto resolve_transition(const std::string &transition, std::vector<std::string> &out) -> bool;

    [[nodiscard]] auto is_colored() const -> bool { return _colored; }

    [[nodiscard]] auto all_colored_place_names() const -> auto & { return _coloredPlaceNames; }
    [[nodiscard]] auto all_colored_transition_names() const -> auto & {
        return _coloredTransitionNames;
    }
};

/** Context provided for evalation */
class EvaluationContext {
  public:
    /** Create evaluation context, this doesn't take ownership */
    EvaluationContext(const MarkVal *marking, const PetriNet *net) : _marking(marking), _net(net) {}

    EvaluationContext(const MarkVal *marking, const PetriNet &net)
        : _marking(marking), _net(&net) {}

    EvaluationContext() = default;

    [[nodiscard]] auto marking() const -> const MarkVal * { return _marking; }

    void set_marking(MarkVal *marking) { _marking = marking; }

    [[nodiscard]] auto net() const -> const PetriNet * { return _net; }

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

    [[nodiscard]] auto negated() const -> bool { return _negated; }

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

    auto marking() const -> const MarkVal * { return _marking; }

    auto net() const -> const PetriNet & { return _net; }

    void negate() { _negated = !_negated; }

    auto negated() const -> bool { return _negated; }

    void set_negate(bool b) { _negated = b; }

    auto get_reduction_time() -> double;

    auto timeout() const -> bool {
        auto end = std::chrono::high_resolution_clock::now();
        auto diff = std::chrono::duration_cast<std::chrono::seconds>(end - _start);
        return (diff.count() >= _queryTimeout);
    }

    auto get_lp_timeout() const -> uint32_t;

    auto make_base_lp() const -> glp_prob *;

  private:
    bool _negated;
    const MarkVal *_marking;
    const PetriNet &_net;
    uint32_t _queryTimeout, _lpTimeout;
    std::chrono::high_resolution_clock::time_point _start;
    mutable glp_prob *_base_lp = nullptr;

    auto build_base() const -> glp_prob *;
};

} // namespace PetriEngine::PQL

#endif // CONTEXTS_H
