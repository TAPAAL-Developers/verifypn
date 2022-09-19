#ifndef EDGE_H
#define EDGE_H

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <forward_list>
#include <unordered_set>

namespace DependencyGraph {

class Configuration;
enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};
enum class EdgeStatus : uint8_t {
    NotWaiting = 0,
    InWaiting = 1,
    Dependency = 2,
    Negation = 3
};

class Edge {
    typedef std::unordered_set<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : source(&t_source) {}

    bool addTarget(Configuration* conf)
    {
        if(handled) return true;
        assert(conf);
        if(conf == source)
        {
            handled = true;
            targets.clear();
            targets.insert(source);
        }
        /*else {
            for (auto* t : targets) {
                if (conf == t) return false;
            }*/
        targets.insert(conf);
        //}
        return handled;
    }

    container targets;
    Configuration* source;
    EdgeStatus status = EdgeStatus::NotWaiting;
    bool processed = false;
    bool is_negated = false;
    bool handled = false;
    int32_t refcnt = 0;
    /*size_t children;
    Assignment assignment;*/

#ifndef NDEBUG
    bool has_suc(size_t id);
#endif
};
}
#endif // EDGE_H
