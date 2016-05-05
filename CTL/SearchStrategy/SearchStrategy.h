#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "../DependencyGraph/Edge.h"

namespace SearchStrategy {

struct Message {
    enum type {HALT = 0, REQUEST = 1, ANSWER = 2};
    DependencyGraph::Configuration *configuration;
};

class AbstractSearchStrategy
{
public:
    virtual void pushEdge(DependencyGraph::Edge *edge) =0;
    virtual void pushNegationEdge(DependencyGraph::Edge *edge) =0;
    virtual void pushMessage(Message &message) =0;

    virtual int pickTask(DependencyGraph::Edge*& edge,
                         DependencyGraph::Edge*& negationEdge,
                         Message*& message,
                         int distance) =0;
};

}
#endif // SEARCHSTRATEGY_H
