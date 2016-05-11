#ifndef MESSAGEWAITINGLIST_H
#define MESSAGEWAITINGLIST_H

#include "iWaitingList.h"
#include <queue>

namespace SearchStrategy {

class MessageWaitingList : public iMassageList{
    std::queue<Message> messages;

    // iWaitingList interface
public:
    virtual bool empty() const { return messages.size(); }
    virtual std::size_t size() const { return messages.size(); }
    virtual bool pop(Message &t)
    {
        if(!messages.empty()){
            t = messages.front();
            messages.pop();
            return true;
        }
        else
            return false;
    }
    virtual void push(Message &t)
    {
        messages.push(t);
    }
};

}

#endif // MESSAGEWAITINGLIST_H
