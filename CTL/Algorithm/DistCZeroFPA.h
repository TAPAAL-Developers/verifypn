#ifndef DISTCZEROFPA_H
#define DISTCZEROFPA_H

#include "FixedPointAlgorithm.h"
#include "PartitionFunction.h"
#include "../Communicator/Communicator.h"

#include <stdlib.h>
#include <queue>
#include <vector>
#include <fstream>

#define FLAG_CLEAN 0
#define FLAG_DIRTY 1
#define FLAG_TERMINATE 2

namespace Algorithm {

class DistCZeroFPA
{
public:
    DistCZeroFPA(PartitionFunction *partition, Communicator *comm);

    virtual bool search(DependencyGraph::BasicDependencyGraph &t_graph,
                        SearchStrategy::iDistributedSearchStrategy &t_strategy);

protected:
    //Negation priority queue
    struct edge_prioritizer{
        bool operator()(const DependencyGraph::Edge *lhs, const DependencyGraph::Edge *rhs) const {
            return (lhs->source->getDistance() > rhs->source->getDistance());
        }
    };
    typedef std::priority_queue<DependencyGraph::Edge*, std::vector<DependencyGraph::Edge*>, Algorithm::DistCZeroFPA::edge_prioritizer> PriorityQueue;

    DependencyGraph::BasicDependencyGraph *graph = nullptr;
    DependencyGraph::Configuration *v = nullptr;

    SearchStrategy::iDistributedSearchStrategy *strategy = nullptr;

    PartitionFunction *partition = nullptr;
    Communicator *comm = nullptr;

    PriorityQueue unsafe_N;

    int termination_flag = FLAG_CLEAN;
    bool waiting_for_token = false;
    long message_counter = 0;

    void finalAssign(DependencyGraph::Configuration *c, DependencyGraph::Assignment value);
    void explore(DependencyGraph::Configuration *c);   
    void halt(DependencyGraph::Configuration *c);
    void processMessage(SearchStrategy::Message *m);
    void processHyperEdge(DependencyGraph::Edge *e);
    void processNegationEdge(DependencyGraph::Edge *e);
    bool terminationDetection();

    //Timestamps
    int messageId = 0;

    int nextMessageId() {
        messageId += 1;
        return messageId;
    }

    ///Debugging functions and variables


    std::fstream ostream;
    template<class ...args>
    void output(args ...arguments);
    std::string to_string();
    template<class T, class ...Ts>
    std::string to_string(T head, Ts... tail);
};
}
#endif // DISTCZEROFPA_H
