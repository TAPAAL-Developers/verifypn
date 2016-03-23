#ifndef COMMUNICATOR_H
#define COMMUNICATOR_H

#include <cstddef>

struct message {
public:
    message(int *t_start = nullptr, int *t_end = nullptr) : start(t_start), end(t_end){}
    int *start;
    int *end;
    std::size_t size() const { return (end - start) / sizeof(int); }
};

class AbstractCommunicator {
    void send(message &msg) =0;
    message* receive() =0;
};

#endif // COMMUNICATOR_H
