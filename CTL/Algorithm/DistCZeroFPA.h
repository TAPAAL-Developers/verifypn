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

    //Timestamps
    int messageId = 0;

    int nextMessageId() {
        messageId += 1;
        return messageId;
    }    
};
}
#endif // DISTCZEROFPA_H
