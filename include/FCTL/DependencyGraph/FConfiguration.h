#ifndef FEATURED_CONFIGURATION_H
#define FEATURED_CONFIGURATION_H

#include "FCTL/DependencyGraph/FeaturedEdge.h"
#include "logging.h"

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
            bool seen_;
#if DEBUG_DETAILED
            size_t id;
#endif

            Configuration() {}

            uint32_t getDistance() const { return distance; }

            [[nodiscard]] bool done() const { return (good | bad) == bddtrue; }

            [[nodiscard]] bool is_seen() const { return seen_; }

            void addDependency(Edge* e);

            void setOwner(uint32_t) {}

            uint32_t getOwner() { return 0; }

            bdd good = bddfalse;
            bdd bad = bddfalse;

        };
        std::string to_string(Assignment a);
    }
}

#endif // CONFIGURATION_H
