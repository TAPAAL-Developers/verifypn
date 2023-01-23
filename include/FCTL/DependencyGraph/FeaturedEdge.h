#ifndef FEATURED_EDGE_H
#define FEATURED_EDGE_H

#include <bddx.h>

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <forward_list>

namespace Featured {
    namespace DependencyGraph {

        class Configuration;

        enum Assignment {
            ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
        };

        class Edge {
        public:
            struct Successor {
                Configuration* conf;
                bdd feat;

                Successor(Configuration* conf, const bdd& feat) : conf(conf), feat(feat) {}
            };
        private:
            typedef std::forward_list<Successor> container;
        public:
            Edge() {}

            Edge(Configuration& t_source) : source(&t_source) {}

            bool addTarget(Configuration* conf, const bdd& feat) {
                if (handled) return true;
                assert(conf);
                if (conf == source) {
                    handled = true;
                    targets.clear();
                } else targets.emplace_front(conf, feat);
                return handled;
            }
            bdd bad_iter = bddfalse;

            container targets;
            Configuration* source;
            uint8_t status = 0;
            bool processed = false;
            bool is_negated = false;
            bool handled = false;
            int32_t refcnt = 0;
        };
    }
}
#endif // EDGE_H
