#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "../DependencyGraph/Edge.h"
#include <sstream>

namespace SearchStrategy {

struct Message {
    enum Type {HALT = 0, REQUEST = 1, ANSWER = 2};
    std::string ToString() {
        std::stringstream ss;
        ss << "Message: ";
        ss << (type == HALT ? "Halt" : type == REQUEST ? "Request" : "Answer");
        ss << configuration << "\n";
        return ss.str();
    }
    Type type;
    DependencyGraph::Configuration *configuration;
};

class AbstractSearchStrategy
{
public:
    enum TaskType {EMPTY = -1, UNAVAILABLE = 0, HYPEREDGE = 1, NEGATIONEDGE = 2, MESSAGE = 3};

    virtual bool empty() =0;

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
