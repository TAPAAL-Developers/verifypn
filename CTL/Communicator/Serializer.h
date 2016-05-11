#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "../SearchStrategy/iSearchStrategy.h"

class Serializer
{
public:
    virtual std::pair<int, int*> serialize(SearchStrategy::Message &m) =0;

    virtual SearchStrategy::Message deserialize(int* message, int messageSize) =0;

};

#endif // SERIALIZER_H
