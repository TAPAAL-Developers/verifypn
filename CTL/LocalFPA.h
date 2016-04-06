#ifndef LOCALFPA_H
#define LOCALFPA_H

#include "FixedPointAlgorithm.h"
#include "edge.h"
#include "configuration.h"

#include <list>
#include <queue>
#include <vector>

namespace ctl {

class LocalFPA : public FixedPointAlgorithm
{
public:
    bool search(DependencyGraph &t_graph, AbstractSearchStrategy &t_strategy);

private:
    struct edge_prioritizer{
        bool operator()(const Edge *lhs, const Edge *rhs) const {
            return (lhs->source->query->max_depth > rhs->source->query->max_depth);
        }
    };
    typedef std::priority_queue<Edge*, std::vector<Edge*>, ctl::LocalFPA::edge_prioritizer> PriorityQueue;
};
}
#endif // LOCALFPA_H
