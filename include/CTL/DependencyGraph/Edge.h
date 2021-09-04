#ifndef EDGE_H
#define EDGE_H

#include <cstdio>
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>
#include <forward_list>

namespace DependencyGraph {

class Configuration;
enum Assignment {
    ONE = 1, UNKNOWN = 0, ZERO = -1, CZERO = -2
};

class Edge {
    typedef std::forward_list<Configuration*> container;
public:
    Edge(){}
    Edge(Configuration &t_source) : _source(&t_source) {}

    void add_target(Configuration* conf)
    {
        assert(conf);
        _targets.push_front(conf);
    }

    container _targets;
    Configuration* _source;
    uint8_t _status = 0;
    bool _processed = false;
    bool _is_negated = false;
    bool _handled = false;
    int32_t _refcnt = 0;
    /*size_t children;
    Assignment assignment;*/
};
}
#endif // EDGE_H
