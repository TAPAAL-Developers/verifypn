#ifndef LOCAL_FP_ALGORITHM_H
#define LOCAL_FP_ALGORITHM_H

#include "fp_algorithm.h"
#include <queue>

namespace ctl {

class Local_FP_Algorithm : public FP_Algorithm
{
    struct edge_prioritizer{
        bool operator()(const Edge *lhs, const Edge *rhs) const {
            return (lhs->source->query->max_depth > rhs->source->query->max_depth);
        }
    };
public:
    Local_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking);

    bool search(CTLTree *t_query, AbstractSearchStrategy *t_W);

private:
    bool local_fp_algorithm(Configuration &v, AbstractSearchStrategy &W);

    typedef std::priority_queue<Edge*, std::vector<Edge*>, ctl::Local_FP_Algorithm::edge_prioritizer> PriorityQueue;
};
}//ctl
#endif // LOCAL_FP_ALGORITHM_H
