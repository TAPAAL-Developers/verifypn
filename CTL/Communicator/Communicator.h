#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <boost/mpi.hpp>

class Communicator
{
public:
    boost::mpi::communicator comm;
    //Rank
    virtual int rank() =0;

    //Size
    virtual int size() =0;

    //Send methods
    virtual void sendMessage(int receiver, int *message, int messageSize) =0;
    virtual void sendToken(int receiver, int *token, int tokenSize);
    virtual void sendDistance(int receiver, int distance);

    //Receive methods
    virtual bool recvMessage(int &sender, int *message, int &messageSize) =0;
    virtual bool recvToken(int &sender, int *token, int &tokenSize) =0;
    virtual bool recvDistance(int &sender, int &distance);

    //Broadcast
    virtual void broadcastDistance(int sender, int *message) =0;
};

#endif // COMMUNICATOR_H
