#ifndef FEATURED_FIXEDPOINTALGORITHM_H
#define FEATURED_FIXEDPOINTALGORITHM_H

#include "FCTL/DependencyGraph/FBasicDependencyGraph.h"
#include "FCTL/SearchStrategy/SearchStrategy.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"

namespace Featured {
    namespace Algorithm {

        class FixedPointAlgorithm {
        public:
            virtual std::pair<bdd, bdd> search(DependencyGraph::BasicDependencyGraph& graph, bool negate=false) = 0;

            FixedPointAlgorithm(Strategy type);

            virtual ~FixedPointAlgorithm() {}

            size_t processedEdges() const { return _processedEdges; }

            size_t processedNegationEdges() const { return _processedNegationEdges; }

            size_t exploredConfigurations() const { return _exploredConfigurations; }

            size_t numberOfEdges() const { return _numberOfEdges; }

        protected:
            std::shared_ptr<SearchStrategy> strategy;
            //total number of processed edges
            size_t _processedEdges = 0;
            //total number of processed negation edges
            size_t _processedNegationEdges = 0;
            //number of explored configurations
            size_t _exploredConfigurations = 0;
            //total number of edges found when computing successors
            size_t _numberOfEdges = 0;
        };
    }
}
#endif // FIXEDPOINTALGORITHM_H
