#include "MPICommunicator.h"

MPICommunicator::MPICommunicator(Serializer *serializer) : serializer(serializer)
{
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &this->_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &this->_size);
}


MPICommunicator::~MPICommunicator()
{
    MPI_Finalize();
}

int MPICommunicator::rank()
{
    return _rank;
}

int MPICommunicator::size()
{
    return _size;
}

void MPICommunicator::reset()
{
    std::pair<int, SearchStrategy::Message> p1;
    do {
        p1 = recvMessage();
    } while (p1.first >= 0);
    std::pair<int, Token> p2;
    do {
        p2 = recvToken(MPI_ANY_SOURCE);
    } while (p2.first >= 0);
}

void MPICommunicator::sendMessage(int receiver, SearchStrategy::Message &m)
{
    int slot = getMPISlot();
    while (slot == NO_SLOT) {
        slot = getMPISlot();
    }

    std::pair<int, int*> serialized = serializer->serialize(m);
    send_requests[slot].buffer = serialized.second;

    MPI_Isend(serialized.second, serialized.first, MPI_INT, receiver, MPI_MESSAGE_TAG, MPI_COMM_WORLD, &send_requests[slot].mpiRequest);
}

void MPICommunicator::sendToken(int receiver, Token &t)
{
    int slot = getMPISlot();
    while (slot == NO_SLOT) {
        slot = getMPISlot();
    }

    int* tokenBuffer = (int*) malloc(sizeof(int*) * 2);
    tokenBuffer[0] = t.flag;
    tokenBuffer[1] = t.messages;
    send_requests[slot].buffer = tokenBuffer;

    MPI_Isend(tokenBuffer, 2, MPI_INT, receiver, MPI_TOKEN_TAG, MPI_COMM_WORLD, &send_requests[slot].mpiRequest);
}

std::pair<int, SearchStrategy::Message> MPICommunicator::recvMessage()
{
    MPI_Status status;
    int has_message = 0;
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_MESSAGE_TAG, MPI_COMM_WORLD, &has_message, &status);
    if (has_message) {
        int size = 0;
        MPI_Get_count(&status, MPI_INT, &size);
        int *buffer = (int*) malloc(sizeof(int) * size);
        MPI_Recv(buffer, size, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        SearchStrategy::Message m = serializer->deserialize(buffer, size);
        delete buffer;
        return std::pair<int, SearchStrategy::Message>(status.MPI_SOURCE, m);
    } else {
        SearchStrategy::Message m(-1, 0, SearchStrategy::Message::HALT, 0, nullptr);
        return std::pair<int, SearchStrategy::Message>(-1, m);
    }
}

std::pair<int, Token> MPICommunicator::recvToken(int source)
{
    MPI_Status status;
    int has_message = 0;
    MPI_Iprobe(source, MPI_TOKEN_TAG, MPI_COMM_WORLD, &has_message, &status);
    Token t(0, 0);
    if (has_message) {
        int buffer[2];
        MPI_Recv(&buffer, 2, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, MPI_COMM_WORLD, &status);
        t.flag = buffer[0];
        t.messages = buffer[1];
        return std::pair<int, Token>(status.MPI_SOURCE, t);
    } else {
        return std::pair<int, Token>(-1, t);
    }
}

void MPICommunicator::computeMax(int &distance)
{
    int winner = distance;
    MPI_Allreduce(&distance, &winner, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    distance = winner;
}

int MPICommunicator::getMPISlot()
{
    // Clear old requests
    MPI_Status status;
    int flag = 1;
    while (send_requests.size() > 0) {
        RequestInfo info = send_requests.front();
        MPI_Test(&info.mpiRequest, &flag, &status);
        if (flag == false)
            break;

        free(info.buffer);
        send_requests.pop_front();
    }

    // Alloc new
    if (send_requests.size() < MAX_SEND_REQUESTS) {
        send_requests.push_back(RequestInfo());
        return send_requests.size() - 1;
    } else {
        return NO_SLOT;
    }
}
