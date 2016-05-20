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

#include "FixedPointAlgorithm.h"

namespace Algorithm {


class DistCZeroFPA : public DistributedFixedPointAlgorithm
{
public:
    virtual bool search(DependencyGraph::BasicDependencyGraph &graph,
                        SearchStrategy::iDistributedSearchStrategy &strategy,
                        Communicator &communicator,
                        PartitionFunction &partition_function) override;

protected:

    DependencyGraph::BasicDependencyGraph *graph = nullptr;
    DependencyGraph::Configuration *v = nullptr;

    SearchStrategy::iDistributedSearchStrategy *strategy = nullptr;

    PartitionFunction *partition = nullptr;
    Communicator *comm = nullptr;

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
    void sendMessage(int receiver, SearchStrategy::Message &m);
    void addDependency(DependencyGraph::Edge *e, DependencyGraph::Configuration *target);

    //total number of processed edges
    long s_edges = 0;
    //total number of processed negation edges
    long s_negation_edges = 0;
    //number of negation vote rounds
    long s_negation_pick_rounds = 0;
    //number of received request messages
    long s_requests = 0;
    //number of received answer messages
    long s_answers = 0;
    //number of halt messages
    long s_halts = 0;
    //number of token messages
    long s_tokens = 0;
    //number of explored configurations
    long s_explored = 0;
    //total number of edges found when computing successors
    long s_total_succ = 0;
    //total number of targets found when computing successors
    long s_total_targets = 0;
    //time spent processing edges and messages
    double s_time_in_processing = 0;
    //time spent receiving messages
    double s_time_in_receive = 0;
    //time spent idle
    double s_time_idle = 0;
    //time in succ generator
    double s_time_generator = 0;

    //Timestamps
    int messageId = 0;

    int nextMessageId() {
        messageId += 1;
        return messageId;
    }    
};
}
#endif // DISTCZEROFPA_H
