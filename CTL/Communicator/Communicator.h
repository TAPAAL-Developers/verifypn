#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include "../SearchStrategy/iSearchStrategy.h"
#include "Token.h"

class Communicator
{
public:
    virtual ~Communicator() {}

    //Rank
    virtual int rank() =0;

    //Size
    virtual int size() =0;

    //Reset
    virtual void reset() =0;

    //Send methods
    virtual void sendMessage(int receiver, SearchStrategy::Message &m) =0;
    virtual void sendToken(int receiver, Token &t) =0;

    //Receive methods
    //first element of the pair is always a sender id. If the id is negative, the opration
    //was not successful
    virtual std::pair<int, SearchStrategy::Message> recvMessage() =0;
    virtual std::pair<int, Token> recvToken(int source) =0;

    //Blocking reduce operation
    //Upon returning, the distance variable contains the maximum distance over all workers
    virtual void computeMax(int &distance) =0;
};

#endif // COMMUNICATOR_H
