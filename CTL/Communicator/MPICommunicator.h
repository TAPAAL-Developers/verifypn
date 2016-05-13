#ifndef MPICOMMUNICATOR_H
#define MPICOMMUNICATOR_H

#include "mpi.h"
#include "Communicator.h"
#include "Serializer.h"

#include <deque>

#define MAX_SEND_REQUESTS 300
#define NO_SLOT 65535

#define MPI_MESSAGE_TAG 1
#define MPI_TOKEN_TAG 2
#define MPI_DISTANCE_TAG 3

class MPICommunicator : public Communicator
{   
public:
    MPICommunicator(Serializer *serializer = nullptr);
    ~MPICommunicator();

    //Rank
    virtual int rank() override;

    //Size
    virtual int size() override;

    //Reset
    virtual void reset() override;

    //Send methods
    virtual void sendMessage(int receiver, SearchStrategy::Message &m) override;
    virtual void sendToken(int receiver, Token &t) override;
    virtual void sendDistance(int receiver, int distance) override;

    //Receive methods
    //first element of the pair is always a sender id. If the id is negative, the opration
    //was not successful
    virtual std::pair<int, SearchStrategy::Message> recvMessage() override;
    virtual std::pair<int, Token> recvToken(int source) override;
    virtual std::pair<int, int> recvDistance() override;

    //Broadcast
    //blocking broadcast - when this method returns, distance
    //variable on wevery worker contains the value which was
    //originally on the sender node.
    virtual void broadcastDistance(int sender, int &distance) override;

protected:
    int _rank = 0;
    int _size = 0;
    Serializer *serializer;

    int getMPISlot();

    struct RequestInfo {
        MPI_Request mpiRequest;
        int* buffer;
    };

    std::deque<RequestInfo> send_requests;

};

#endif // MPICOMMUNICATOR_H
