#ifndef SEARCHSTRATEGY_H
#define SEARCHSTRATEGY_H

#include "../DependencyGraph/Edge.h"
#include <sstream>

namespace SearchStrategy {

struct Message {    
    enum Type {HALT = 0, REQUEST = 1, ANSWER_ONE = 2, ANSWER_ZERO = 3} type;
    unsigned long id;
    DependencyGraph::Configuration *configuration;

    Message(Type type, unsigned long id, DependencyGraph::Configuration *configuration) :
           type(type), id(id), configuration(configuration) {}

    std::string ToString() {
        std::stringstream ss;
        ss << "Message: ";
        ss << (type == HALT ? "Halt" : type == REQUEST ? "Request" : type == ANSWER_ONE ? "Answer 1" : "Answer 0");
        ss << configuration << "\n";
        return ss.str();
    }
};

class AbstractSearchStrategy
{
public:
    enum TaskType {EMPTY = 0, EDGE = 1, MESSAGE = 2};
    virtual bool empty() =0;

    virtual void pushEdge(DependencyGraph::Edge *edge) =0;
    virtual void pushMessage(Message &message) =0;

    virtual TaskType pickTask(DependencyGraph::Edge*& edge,
                         Message*& message) =0;

};

}
#endif // SEARCHSTRATEGY_H
