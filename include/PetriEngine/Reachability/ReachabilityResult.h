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
#ifndef REACHABILITYRESULT_H
#define REACHABILITYRESULT_H

#include "../PQL/PQL.h"
#include "../Reducer.h"
#include "../Structures/StateSet.h"
#include <vector>

struct options_t;

namespace PetriEngine {
class PetriNetBuilder;
namespace Reachability {

/** Result of a reachability search */

class AbstractHandler {
  public:
    enum Result {
        /** The query was satisfied */
        SATISFIED,
        /** The query cannot be satisfied */
        NOT_SATISFIED,
        /** We're unable to say if the query can be satisfied */
        UNKNOWN,
        /** The query should be verified using the CTL engine */
        CTL,
        /** The query should be verified using the LTL engine */
        LTL,
        /** Just ignore */
        IGNORE
    };
    virtual auto handle(size_t index, const PQL::Condition &query, Result result,
                        const std::vector<uint32_t> *maxPlaceBound = nullptr,
                        size_t expandedStates = 0, size_t exploredStates = 0,
                        size_t discoveredStates = 0, int maxTokens = 0,
                        const Structures::StateSetInterface *stateset = nullptr,
                        size_t lastmarking = 0, const MarkVal *initialMarking = nullptr) const
        -> std::pair<Result, bool> = 0;
};

class ResultPrinter : public AbstractHandler {
  protected:
    const PetriNetBuilder &_builder;
    const options_t &_options;
    const std::vector<std::string> &_querynames;
    const Reducer *_reducer;

  public:
    const std::string _techniques =
        "TECHNIQUES COLLATERAL_PROCESSING STRUCTURAL_REDUCTION QUERY_REDUCTION SAT_SMT ";
    const std::string _techniquesStateSpace = "TECHNIQUES EXPLICIT STATE_COMPRESSION";

    ResultPrinter(const PetriNetBuilder &b, const options_t &o,
                  const std::vector<std::string> &querynames)
        : _builder(b), _options(o), _querynames(querynames), _reducer(nullptr){};

    void set_reducer(const Reducer &r) { this->_reducer = &r; }

    auto handle(size_t index, const PQL::Condition &query, Result result,
                const std::vector<uint32_t> *maxPlaceBound = nullptr, size_t expandedStates = 0,
                size_t exploredStates = 0, size_t discoveredStates = 0, int maxTokens = 0,
                const Structures::StateSetInterface *stateset = nullptr, size_t lastmarking = 0,
                const MarkVal *initialMarking = nullptr) const -> std::pair<Result, bool> override;

    [[nodiscard]] auto print_techniques() const -> std::string;

    void print_trace(const Structures::StateSetInterface &, size_t lastmarking) const;
};
} // namespace Reachability
} // namespace PetriEngine

#endif // REACHABILITYRESULT_H
