#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "../SearchStrategy/iSearchStrategy.h"
#include "Token.h"

class Communicator
{
public:
    //Rank
    virtual int rank() =0;

    //Size
    virtual int size() =0;

    //Reset
    virtual void reset() =0;

    //Send methods
    virtual void sendMessage(int receiver, SearchStrategy::Message &m) =0;
    virtual void sendToken(int receiver, Token &t) =0;
    virtual void sendDistance(int receiver, int distance) =0;

    //Receive methods
    //first element of the pair is always a sender id. If the id is negative, the opration
    //was not successful
    virtual std::pair<int, SearchStrategy::Message> recvMessage() =0;
    virtual std::pair<int, Token> recvToken(int source) =0;
    virtual std::pair<int, int> recvDistance() =0;

    //Broadcast
    //blocking broadcast - when this method returns, distance
    //variable on wevery worker contains the value which was
    //originally on the sender node.
    virtual void broadcastDistance(int sender, int &distance) =0;
};

#endif // COMMUNICATOR_H
