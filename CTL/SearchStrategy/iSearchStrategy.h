#ifndef ISEARCHSTRATEGY_H
#define ISEARCHSTRATEGY_H

#include "../DependencyGraph/Edge.h"
#include <sstream>

namespace SearchStrategy {

enum TaskType {EMPTY = -1, UNAVAILABLE = 0, EDGE = 1, MESSAGE = 2};

struct Message {
    enum Type {HALT = 0, REQUEST = 1, ANSWER = 2} type;
    unsigned long id;
    DependencyGraph::Configuration *configuration;

    std::string ToString() {
        std::stringstream ss;
        ss << "Message: ";
        ss << (type == HALT ? "Halt" : type == REQUEST ? "Request" : "Answer");
        ss << configuration << "\n";
        return ss.str();
    }
};

class iClearable {
    virtual void clear() =0;
};

class iSequantialSearchStrategy{
public:
    virtual bool empty() =0;
    virtual void pushEdge(DependencyGraph::Edge *edge) =0;
    virtual void pushMessage(Message &message) =0;

    virtual TaskType pickTask(DependencyGraph::Edge*& edge) =0;
};

class iDistributedSearchStrategy
{
public:
    virtual bool empty() =0;
    virtual unsigned int maxDistance() =0;
    virtual void pushEdge(DependencyGraph::Edge *edge) =0;
    virtual void pushMessage(Message &message) =0;
    virtual void releaseNegationEdges(int dist) =0;

    virtual TaskType pickTask(DependencyGraph::Edge*& edge,
                              Message*& message) =0;
};

}
#endif // SEARCHSTRATEGY_H
