#ifndef ROUND_H
#define ROUND_H

#include "Position.h"

namespace DependencyGraph {

class Edge;
class Configuration;

namespace Game {

class GameRound{
public:
    int round = 0;
    GamePosition position;
    Edge *edge = nullptr;
    Configuration *configuration = nullptr;

    inline bool operator==(const GameRound &rhs) const {
        return position == rhs.position;
    }
    inline bool operator!=(const GameRound &rhs) const {
        return !((*this)==rhs);
    }
    struct hasher {
        std::size_t operator()(const GameRound &round) const {
            return (uintptr_t)round.position.configuration ^ (uintptr_t)round.configuration ^ (uintptr_t)round.edge;
        }
    };
};
}}

#endif // ROUND_H
