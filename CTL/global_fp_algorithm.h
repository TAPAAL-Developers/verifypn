#ifndef GLOBAL_FP_ALGORITHM_H
#define GLOBAL_FP_ALGORITHM_H

#include "fp_algorithm.h"
#include <queue>

namespace ctl {

class Global_FP_Algorithm : public FP_Algorithm
{   
public:
    struct edge_prioritizer{
        bool operator()(const Edge *lhs, const Edge *rhs) const {
            return (lhs->source->query->max_depth > rhs->source->query->max_depth);
        }
    };

    Global_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking);
    bool search(CTLTree *t_query, EdgePicker *t_W);
    bool search(CTLTree *t_query, EdgePicker *t_W, CircleDetector *t_detector);

protected:
    bool global_fp_algorithm(Configuration &v,
                            EdgePicker &W,
                            bool cycle_detection = false,
                            CircleDetector *detector = NULL);


    typedef std::priority_queue<Edge*, std::vector<Edge*>, ctl::Global_FP_Algorithm::edge_prioritizer> PriorityQueue;
    void buildDependencyGraph(Configuration &v);
    void CalculateEdges(Configuration &v, EdgePicker &W);
};
}//ctl
#endif // GLOBAL_FP_ALGORITHM_H
