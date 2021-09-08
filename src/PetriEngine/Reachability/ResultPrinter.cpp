
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"

#include "options.h"

namespace PetriEngine::Reachability {
auto ResultPrinter::handle(size_t index, const PQL::Condition &query, Result result,
                           const std::vector<uint32_t> *maxPlaceBound, size_t expandedStates,
                           size_t exploredStates, size_t discoveredStates, int maxTokens,
                           const Structures::StateSetInterface *stateset, size_t lastmarking,
                           const MarkVal *initialMarking) const
    -> std::pair<AbstractHandler::Result, bool> {
    if (result == UNKNOWN)
        return std::make_pair(UNKNOWN, false);

    Result retval = result;

    if (_options._cpn_overapprox) {
        if (query.get_quantifier() == PQL::quantifier_e::UPPERBOUNDS) {
            auto upq = ((const PQL::UnfoldedUpperBoundsCondition &)query);
            auto bnd = upq.bounds();
            if (initialMarking == nullptr || bnd > upq.value(initialMarking))
                retval = UNKNOWN;
        } else if (result == SATISFIED)
            retval = UNKNOWN;
        else if (result == NOT_SATISFIED)
            retval = query.is_invariant() ? SATISFIED : NOT_SATISFIED;
        if (retval == UNKNOWN) {
            std::cout << "\nUnable to decide if " << _querynames[index] << " is satisfied.\n\n";
            std::cout << "Query is MAYBE satisfied.\n" << std::endl;
            return std::make_pair(IGNORE, false);
        }
    }
    std::cout << std::endl;

    bool showTrace = (result == SATISFIED);

    if (!_options._statespace_exploration && retval != UNKNOWN) {
        std::cout << "FORMULA " << _querynames[index] << " ";
    } else {
        retval = SATISFIED;
        uint32_t placeBound = 0;
        if (maxPlaceBound != nullptr) {
            for (unsigned int p : *maxPlaceBound) {
                placeBound = std::max<uint32_t>(placeBound, p);
            }
        }
        // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n",
        // result.exploredStates(), result.maxTokens(), placeBound);
        std::cout << "STATE_SPACE STATES " << exploredStates << " " << _techniquesStateSpace
                  << std::endl
                  << "STATE_SPACE TRANSITIONS " << -1 << " " << _techniquesStateSpace << std::endl
                  << "STATE_SPACE MAX_TOKEN_PER_MARKING " << maxTokens << " "
                  << _techniquesStateSpace << std::endl
                  << "STATE_SPACE MAX_TOKEN_IN_PLACE " << placeBound << " " << _techniquesStateSpace
                  << std::endl;
        return std::make_pair(retval, false);
    }

    if (result == SATISFIED)
        retval = query.is_invariant() ? NOT_SATISFIED : SATISFIED;
    else if (result == NOT_SATISFIED)
        retval = query.is_invariant() ? SATISFIED : NOT_SATISFIED;

    // Print result
    auto bound = &query;
    if (auto ef = dynamic_cast<const PQL::EFCondition *>(&query)) {
        bound = (*ef)[0].get();
    }
    bound = dynamic_cast<const PQL::UnfoldedUpperBoundsCondition *>(bound);

    if (retval == UNKNOWN) {
        std::cout << "\nUnable to decide if " << _querynames[index] << " is satisfied.";
    } else if (bound) {
        std::cout << ((PQL::UnfoldedUpperBoundsCondition *)bound)->bounds() << " " << _techniques
                  << print_techniques() << std::endl;
        std::cout << "Query index " << index << " was solved" << std::endl;
    } else if (retval == SATISFIED) {
        if (!_options._statespace_exploration) {
            std::cout << "TRUE " << _techniques << print_techniques() << std::endl;
            std::cout << "Query index " << index << " was solved" << std::endl;
        }
    } else if (retval == NOT_SATISFIED) {
        if (!_options._statespace_exploration) {
            std::cout << "FALSE " << _techniques << print_techniques() << std::endl;
            std::cout << "Query index " << index << " was solved" << std::endl;
        }
    }

    std::cout << std::endl;

    std::cout << "Query is ";
    if (_options._statespace_exploration) {
        retval = SATISFIED;
    }

    if (result == SATISFIED)
        retval = query.is_invariant() ? NOT_SATISFIED : SATISFIED;
    else if (result == NOT_SATISFIED)
        retval = query.is_invariant() ? SATISFIED : NOT_SATISFIED;

    // Print result
    if (retval == UNKNOWN) {
        std::cout << "MAYBE ";
    } else if (retval == NOT_SATISFIED) {
        std::cout << "NOT ";
    }
    std::cout << "satisfied." << std::endl;

    if (_options._cpn_overapprox)
        std::cout << "\nSolved using CPN Approximation\n" << std::endl;

    if (showTrace && _options._trace != options_t::trace_level_e::NONE) {
        if (stateset == nullptr) {
            std::cout << "No trace could be generated" << std::endl;
        } else {
            print_trace(*stateset, lastmarking);
        }
    }

    std::cout << std::endl;
    return std::make_pair(retval, false);
}

auto ResultPrinter::print_techniques() const -> std::string {
    std::string out;

    if (_options._query_reduction_timeout > 0) {
        out += "LP_APPROX ";
    }

    if (_options._cpn_overapprox) {
        out += "CPN_APPROX ";
    }

    if (_options._is_CPN && !_options._cpn_overapprox) {
        out += "UNFOLDING_TO_PT ";
    }

    if (_options._query_reduction_timeout == 0 && !_options._tar &&
        _options._siphontrap_timeout == 0) {
        out += "EXPLICIT STATE_COMPRESSION ";
        if (_options._stubborn_reduction) {
            out += "STUBBORN_SETS ";
        }
    }
    if (_options._tar) {
        out += "TRACE_ABSTRACTION_REFINEMENT ";
    }
    if (_options._siphontrap_timeout > 0) {
        out += "TOPOLOGICAL SIPHON_TRAP ";
    }

    return out;
}

void ResultPrinter::print_trace(const Structures::StateSetInterface &ss, size_t lastmarking) const {
    std::cerr << "Trace:\n<trace>\n";
    std::stack<size_t> transitions;
    size_t next = lastmarking;
    while (next != 0) // assume 0 is the index of the first marking.
    {
        // (parent, transition)
        std::pair<size_t, size_t> p = ss.get_history(next);
        next = p.first;
        transitions.push(p.second);
    }

    if (_reducer != nullptr)
        _reducer->init_fire(std::cerr);

    while (transitions.size() > 0) {
        size_t trans = transitions.top();
        transitions.pop();
        std::string tname = ss.net().transition_names()[trans];
        std::cerr << "\t<transition id=\"" << tname << "\" index=\"" << trans << "\">\n";

        // well, yeah, we are not really efficient in constructing the trace.
        // feel free to improve
        for (size_t p = 0; p < ss.net().number_of_places(); ++p) {
            size_t cnt = ss.net().in_arc(p, trans);
            for (size_t token = 0; token < cnt; ++token) {
                std::cerr << "\t\t<token place=\"" << ss.net().place_names()[p]
                          << "\" age=\"0\"/>\n";
            }
        }

        if (_reducer != nullptr)
            _reducer->extra_consume(std::cerr, tname);

        std::cerr << "\t</transition>\n";

        if (_reducer != nullptr)
            _reducer->post_fire(std::cerr, tname);
    }

    std::cerr << "</trace>\n" << std::endl;
}

} // namespace PetriEngine::Reachability
