#ifndef ISEARCHSTRATEGY_H
#define ISEARCHSTRATEGY_H

#include "../DependencyGraph/Edge.h"
#include <sstream>

namespace SearchStrategy {

enum TaskType {EMPTY = -1, UNAVAILABLE = 0, EDGE = 1, MESSAGE = 2};

struct Message {    
    int sender;
    enum Type {HALT = 0, REQUEST = 1, ANSWER_ONE = 2, ANSWER_ZERO = 3} type;
    unsigned long id;
    DependencyGraph::Configuration *configuration;

    Message() {}
    Message(int sender, Type type, unsigned long id, DependencyGraph::Configuration *configuration) :
           sender(sender), type(type), id(id), configuration(configuration) {}

    std::string ToString() {
        std::stringstream ss;
        ss << "Message from " << sender << ": ";
        ss << (type == HALT ? "Halt" : type == REQUEST ? "Request" : type == ANSWER_ONE ? "Answer 1" : "Answer 0");
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
