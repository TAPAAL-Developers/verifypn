#ifndef SERIALIZER_H
#define SERIALIZER_H

#include "../SearchStrategy/SearchStrategy.h"

class Serializer
{
public:
    Serializer();

    std::pair<int, int*> serialize(SearchStrategy::Message &m);

    SearchStrategy::Message* deserialize(int* message, int messageSize);

};

#endif // SERIALIZER_H
