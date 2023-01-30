#ifndef FEATURED_ABSTRACTDEPENDENCYGRAPH_H
#define FEATURED_ABSTRACTDEPENDENCYGRAPH_H

#include <cstddef>
#include <vector>
#include <cstdint>
#include <iostream>

namespace Featured {
    namespace DependencyGraph {

        class Configuration;

        class Edge;

        class BasicDependencyGraph {

        public:
            virtual std::vector<Edge*> successors(Configuration* c) = 0;

            virtual Configuration* initialConfiguration() = 0;

            virtual void release(Edge* e) = 0;

            virtual void cleanUp() = 0;

            virtual void print(Configuration* c, std::ostream& out = std::cerr) = 0;

        };

    }
}
#endif // ABSTRACTDEPENDENCYGRAPH_H
