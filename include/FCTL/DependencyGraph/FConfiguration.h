#ifndef FEATURED_CONFIGURATION_H
#define FEATURED_CONFIGURATION_H

#include "FeaturedEdge.h"

#include <bddx.h>

#include <string>
#include <cstdio>
#include <iostream>
#include <vector>
#include <forward_list>

namespace Featured {
    namespace DependencyGraph {

        class Edge;

        class Configuration {
        public:
            std::forward_list<Edge*> dependency_set;
            std::forward_list<Edge*> successors;
            uint32_t nsuccs = 0;
        private:
            uint32_t distance = 0;

            void setDistance(uint32_t value) { distance = value; }

        public:
            int8_t assignment = UNKNOWN;
            bool checked;

            Configuration() {}

            uint32_t getDistance() const { return distance; }

            bool isDone() const { return (good | bad) == bddtrue; }

            void addDependency(Edge* e);

            void setOwner(uint32_t) {}

            uint32_t getOwner() { return 0; }

            bdd good = bddfalse;
            bdd bad = bddfalse;

        };
    }
}

#endif // CONFIGURATION_H