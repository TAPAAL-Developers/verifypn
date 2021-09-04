
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/Expressions.h"

#include "options.h"

namespace PetriEngine {
    namespace Reachability {
        std::pair<AbstractHandler::Result, bool> ResultPrinter::handle(
                size_t index,
                PQL::Condition* query,
                Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking) const
        {
            if(result == Unknown) return std::make_pair(Unknown,false);

            Result retval = result;

            if(_options->_cpn_overapprox)
            {
                if(query->getQuantifier() == PQL::Quantifier::UPPERBOUNDS)
                {
                    auto upq = ((PQL::UnfoldedUpperBoundsCondition*)query);
                    auto bnd = upq->bounds();
                    if(initialMarking == nullptr || bnd > upq->value(initialMarking))
                        retval = Unknown;
                }
                else if (result == Satisfied)
                    retval = query->isInvariant() ? Unknown : Unknown;
                else if (result == NotSatisfied)
                    retval = query->isInvariant() ? Satisfied : NotSatisfied;
                if(retval == Unknown)
                {
                    std::cout << "\nUnable to decide if " << _querynames[index] << " is satisfied.\n\n";
                    std::cout << "Query is MAYBE satisfied.\n" << std::endl;
                    return std::make_pair(Ignore,false);
                }
            }
            std::cout << std::endl;

            bool showTrace = (result == Satisfied);

            if(!_options->_statespace_exploration && retval != Unknown)
            {
                std::cout << "FORMULA " << _querynames[index] << " ";
            }
            else {
                retval = Satisfied;
                uint32_t placeBound = 0;
                if(maxPlaceBound != nullptr)
                {
                    for (size_t p = 0; p < maxPlaceBound->size(); p++) {
                        placeBound = std::max<uint32_t>(placeBound, (*maxPlaceBound)[p]);
                    }
                }
                // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
                std::cout   << "STATE_SPACE STATES "<< exploredStates           << " " << _techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE TRANSITIONS "<< -1                  << " " << _techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE MAX_TOKEN_PER_MARKING "<< maxTokens << " " << _techniquesStateSpace
                            << std::endl
                            << "STATE_SPACE MAX_TOKEN_IN_PLACE "<< placeBound   << " " << _techniquesStateSpace
                            << std::endl;
                return std::make_pair(retval,false);
            }

            if (result == Satisfied)
                retval = query->isInvariant() ? NotSatisfied : Satisfied;
            else if (result == NotSatisfied)
                retval = query->isInvariant() ? Satisfied : NotSatisfied;

            //Print result
            auto bound = query;
            if(auto ef = dynamic_cast<PQL::EFCondition*>(query))
            {
                bound = (*ef)[0].get();
            }
            bound = dynamic_cast<PQL::UnfoldedUpperBoundsCondition*>(bound);

            if (retval == Unknown)
            {
                std::cout << "\nUnable to decide if " << _querynames[index] << " is satisfied.";
            }
            else if(bound)
            {
                std::cout << ((PQL::UnfoldedUpperBoundsCondition*)bound)->bounds() << " " << _techniques << print_techniques() << std::endl;
                std::cout << "Query index " << index << " was solved" << std::endl;
            }
            else if (retval == Satisfied) {
                if(!_options->_statespace_exploration)
                {
                    std::cout << "TRUE " << _techniques << print_techniques() << std::endl;
                    std::cout << "Query index " << index << " was solved" << std::endl;
                }
            } else if (retval == NotSatisfied) {
                if(!_options->_statespace_exploration)
                {
                    std::cout << "FALSE " << _techniques << print_techniques() << std::endl;
                    std::cout << "Query index " << index << " was solved" << std::endl;
                }
            }

            std::cout << std::endl;

            std::cout << "Query is ";
            if(_options->_statespace_exploration)
            {
                retval = Satisfied;
            }

            if (result == Satisfied)
                retval = query->isInvariant() ? NotSatisfied : Satisfied;
            else if (result == NotSatisfied)
                retval = query->isInvariant() ? Satisfied : NotSatisfied;

            //Print result
            if (retval == Unknown)
            {
                std::cout << "MAYBE ";
            }
            else if (retval == NotSatisfied) {
                std::cout << "NOT ";
            }
            std::cout << "satisfied." << std::endl;

            if(_options->_cpn_overapprox)
                std::cout << "\nSolved using CPN Approximation\n" << std::endl;

            if (showTrace && _options->_trace != options_t::trace_level_e::None)
            {
                if(stateset == nullptr)
                {
                    std::cout << "No trace could be generated" << std::endl;
                }
                else
                {
                    print_trace(stateset, lastmarking);
                }
            }

            std::cout << std::endl;
            return std::make_pair(retval, false);
        }

        std::string ResultPrinter::print_techniques() const {
            std::string out;

            if(_options->_query_reduction_timeout > 0)
            {
                out += "LP_APPROX ";
            }

            if(_options->_cpn_overapprox)
            {
                out += "CPN_APPROX ";
            }

            if(_options->_is_CPN && !_options->_cpn_overapprox)
            {
                out += "UNFOLDING_TO_PT ";
            }

            if(_options->_query_reduction_timeout == 0
			    && !_options->_tar
			    && _options->_siphontrap_timeout == 0)
            {
                out += "EXPLICIT STATE_COMPRESSION ";
                if(_options->_stubborn_reduction)
                {
                    out += "STUBBORN_SETS ";
                }
            }
            if(_options->_tar)
            {
                out += "TRACE_ABSTRACTION_REFINEMENT ";
            }
            if(_options->_siphontrap_timeout > 0)
            {
                out += "TOPOLOGICAL SIPHON_TRAP ";
            }

            return out;
        }

        void ResultPrinter::print_trace(Structures::StateSetInterface* ss, size_t lastmarking) const
        {
            std::cerr << "Trace:\n<trace>\n";
            std::stack<size_t> transitions;
            size_t next = lastmarking;
            while(next != 0) // assume 0 is the index of the first marking.
            {
                // (parent, transition)
                std::pair<size_t, size_t> p = ss->get_history(next);
                next = p.first;
                transitions.push(p.second);
            }

            if(_reducer != NULL)
                _reducer->init_fire(std::cerr);

            while(transitions.size() > 0)
            {
                size_t trans = transitions.top();
                transitions.pop();
                std::string tname = ss->net().transition_names()[trans];
                std::cerr << "\t<transition id=\"" << tname << "\" index=\"" << trans << "\">\n";

                // well, yeah, we are not really efficient in constructing the trace.
                // feel free to improve
                for(size_t p = 0; p < ss->net().number_of_places(); ++p)
                {
                    size_t cnt = ss->net().in_arc(p, trans);
                    for(size_t token = 0; token < cnt; ++token )
                    {
                        std::cerr << "\t\t<token place=\"" << ss->net().place_names()[p] << "\" age=\"0\"/>\n";
                    }
                }

                if(_reducer != NULL)
                    _reducer->extra_consume(std::cerr, tname);

                std::cerr << "\t</transition>\n";

                if(_reducer != NULL)
                    _reducer->post_fire(std::cerr, tname);

            }

            std::cerr << "</trace>\n" << std::endl;
        }

    }
}
