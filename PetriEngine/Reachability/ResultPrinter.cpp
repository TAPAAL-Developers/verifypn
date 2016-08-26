
#include "ReachabilityResult.h"
#include "../PetriNetBuilder.h"
#include "../options.h"

namespace PetriEngine {
    namespace Reachability {
        ResultPrinter::Result ResultPrinter::printResult(
                size_t index,
                PQL::Condition* query, 
                ResultPrinter::Result result,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                const std::vector<size_t> enabledTransitionsCount,
                int maxTokens,
                const std::vector<uint32_t> maxPlaceBound )
        {
            if(result == Unknown) return Unknown;
            Result retval = result;
            std::cout << std::endl;      
            if(options->mccoutput)
            {
                if(!options->statespaceexploration && retval != Unknown)
                {
                    std::cout << "FORMULA " << querynames[index] << " ";
                }
                else {
                    retval = Satisfied;
                    uint32_t placeBound = 0;
                    for (size_t p = 0; p < maxPlaceBound.size(); p++) {
                        placeBound = std::max<uint32_t>(placeBound, maxPlaceBound[p]);
                    }
                    // fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
                    std::cout   << "STATE_SPACE STATES "<< exploredStates           << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
    //                            << "STATE_SPACE TRANSITIONS "<< discoveredStates <<" TECHNIQUES EXPLICIT\n" 
                                << "STATE_SPACE TRANSITIONS "<< -1                  << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
                                << "STATE_SPACE MAX_TOKEN_PER_MARKING "<< maxTokens << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n" 
                                << "STATE_SPACE MAX_TOKEN_IN_PLACE "<< placeBound   << " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION STATE_COMPRESSION\n"
                                << std::endl;
                    return retval;
                }

                if (result == Satisfied)
                    retval = query->isInvariant() ? NotSatisfied : Satisfied;
                else if (result == NotSatisfied)
                    retval = query->isInvariant() ? Satisfied : NotSatisfied;

                //Print result
                if (retval == Unknown)
                {
                    std::cout << "\nUnable to decide if " << querynames[index] << " is satisfied.";
                }
                else if (retval == Satisfied) {
                    if(!options->statespaceexploration)
                    {
                        std::cout << "TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";
    /*                    if(options->enablereduction > 0)
                        {*/
                            std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                    }
                        std::cout << std::endl;
                    }
                } else if (retval == NotSatisfied) {
                    if (!query->placeNameForBound().empty()) {
                        // find index of the place for reporting place bound

                        std::cout << query->getBound() <<  " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";

    //                    if(options->enablereduction > 0)
    //                    {
                            std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                    }
                    } else {
                        if(!options->statespaceexploration)
                        {
                            std::cout << "FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT ";
    //                        if(options->enablereduction > 0)
    //                        {
                                std::cout << "STRUCTURAL_REDUCTION STATE_COMPRESSION";
    //                        }
                        }
                    }
                }
            }
            else
            {
                std::cout << "Query is ";
                if(options->statespaceexploration)
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
            }
            std::cout << std::endl;
            return retval;
        }            

    }
}