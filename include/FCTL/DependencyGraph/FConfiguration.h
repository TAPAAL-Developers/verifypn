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
//#if DEBUG_DETAILED
            size_t id;
//#endif

            Configuration() {}

            uint32_t getDistance() const { return distance; }

            [[nodiscard]] bool done() const { return (good | bad) == bddtrue; }

            [[nodiscard]] bool is_seen() const { return !successors.empty() || done(); }

            [[nodiscard]] bool unimproved() const {
                return (good == bddfalse) && (bad == bddfalse);
            }

            void addDependency(Edge* e);

            bool remove_suc(Edge* e, bool);

            void setOwner(uint32_t) {}

            uint32_t getOwner() { return 0; }

            bdd good = bddfalse;
            bdd bad = bddfalse;

            [[nodiscard]] long num_successors() const
            {
                return std::count_if(successors.begin(), successors.end(), [](const auto& e){ return true; });
            }
        };

        [[nodiscard]] static bool operator<(const Configuration& c, const Configuration& v)
        {
            if (c.unimproved() && !v.unimproved()) {
                return true;
            }
            if (!c.done() && v.done()) {
                return true;
            }
            if (!c.is_seen() && v.is_seen()) {
                return true;
            }
            return false;
        }

        std::string to_string(Assignment a);
    }
}

#endif // CONFIGURATION_H
