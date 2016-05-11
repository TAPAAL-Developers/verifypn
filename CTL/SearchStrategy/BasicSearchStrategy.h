#ifndef BASICSEARCHSTRATEGY_H
#define BASICSEARCHSTRATEGY_H

#include "iSearchStrategy.h"
#include "iWaitingList.h"
#include <boost/unordered_map.hpp>

#include <iostream>

namespace SearchStrategy {

class BasicSearchStrategy : public iSequantialSearchStrategy {
//    using Edge = DependencyGraph::Edge;
//    using EdgeContainer = EdgeWaitingList<std::stack<Edge*>>;

//    //Container for regular edges
//    EdgeContainer W;
//    //Container for messages
//    Message m;
//    std::queue<Message> M;


//    // AbstractSearchStrategy interface
//public:

//    virtual bool empty() { return W.empty() && M.empty(); }
//    virtual void clear() { }
//    virtual void pushEdge(DependencyGraph::Edge *edge) { W.push(edge);}
//    virtual void pushMessage(Message &message) {M.push(message);}

//    virtual TaskType pickTask(DependencyGraph::Edge *&edge, Message *&message)
//    {
//        if(!M.empty()){
//            m = M.front();  //Copy message
//            M.pop();        //Pop message
//            message = &m;  //Set pointer to copy of message
//            return MESSAGE;
//        }
//        else if(!W.empty()) {
//            edge = W.top();
//            W.pop();
//            return EDGE;
//        }
//        else
//            return EMPTY;
//    }
};
}
#endif // BASICSEARCHSTRATEGY_H
