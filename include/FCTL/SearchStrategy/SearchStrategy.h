#ifndef FEATURED_ISEARCHSTRATEGY_H
#define FEATURED_ISEARCHSTRATEGY_H

#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include <sstream>

namespace Featured {
    class SearchStrategy {
    public:
        virtual ~SearchStrategy() {};

        bool empty() const;

        void pushEdge(DependencyGraph::Edge* edge);

        void pushDependency(DependencyGraph::Edge* edge);

        void pushNegation(DependencyGraph::Edge* edge);

        DependencyGraph::Edge* popEdge(bool saturate = false);

        size_t size() const;

//#ifdef VERIFYPNDIST
        uint32_t maxDistance() const;

        bool available() const;

        void releaseNegationEdges(uint32_t);

        bool trivialNegation();

        virtual void flush() {};
//#endif
    protected:
        virtual size_t Wsize() const = 0;

        virtual void pushToW(DependencyGraph::Edge* edge) = 0;

        virtual DependencyGraph::Edge* popFromW() = 0;

        std::vector<DependencyGraph::Edge*> N;
        std::vector<DependencyGraph::Edge*> D;
    };
}
#endif // SEARCHSTRATEGY_H
