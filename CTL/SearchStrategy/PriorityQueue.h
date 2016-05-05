#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <deque>
#include <queue>
#include <mutex>
#include "AbstractSearchStrategy.h"
#include "../edge.h"
#include "../configuration.h"

namespace ctl {


struct max_depth_edge_prioritizer {
    bool operator()(const Edge *lhs, const Edge *rhs) const {
        return (lhs->source->query->max_depth > rhs->source->query->max_depth);
    }
};

struct depth_edge_prioritizer {
    bool operator()(const Edge *lhs, const Edge *rhs) const {
        return (lhs->source->query->depth > rhs->source->query->depth);
    }
};

template <typename T>
class PriorityQueue : public AbstractSearchStrategy
{
public:
    PriorityQueue() {}

    // AbstractSearchStrategy interface
public:
    AbstractSearchStrategy *create_clean() {
        return new PriorityQueue<T>();
    }

    void push(Edge *e) {
        Q.push(e);
    }

    void push_dependency(Edge *e) {
        Q.push(e);
    }

    Edge *pop() {
        auto e = Q.top();
        Q.pop();
        return e;
    }

    bool remove(Edge *e) {
        //Not supported by the underlying data structure
        return false;
    }

    size_t size() {
        return Q.size();
    }

    bool empty() {
        return Q.empty();
    }

    void clear() {
        while(!Q.empty()) {
            Q.pop();
        }
    }

    Edge *top() {
        return Q.top();
    }

private:
    typedef std::priority_queue<Edge*, std::deque<Edge*>, T> Queue;
    Queue Q;
};
}
#endif // PRIORITYQUEUE_H
