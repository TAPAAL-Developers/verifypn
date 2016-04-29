#ifndef CERTAINZEROFPA_H
#define CERTAINZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "edge.h"
#include "configuration.h"

#include <list>
#include <queue>
#include <vector>

namespace ctl {

class CertainZeroFPA : public FixedPointAlgorithm
{
public:
    bool search(DependencyGraph &t_graph, AbstractSearchStrategy &t_strategy);

private:
    struct edge_prioritizer{
        bool operator()(const Edge *lhs, const Edge *rhs) const {
            return (lhs->source->query->Depth > rhs->source->query->Depth); //Changed to depth - was max depth
        }
    };
    typedef std::priority_queue<Edge*, std::vector<Edge*>, ctl::CertainZeroFPA::edge_prioritizer> PriorityQueue;
};
}
#endif // CERTAINZEROFPA_H
