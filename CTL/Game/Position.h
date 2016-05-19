#ifndef POSITION_H
#define POSITION_H

#include <cstdint>

namespace DependencyGraph {

class Configuration;

namespace Game {

class GamePosition {
public:
    Configuration *configuration = nullptr;
    int claim = -1;

    GamePosition(Configuration *conf = nullptr, int c = -1) : configuration(conf), claim(c) {}

    inline bool operator==(const GamePosition &rhs) const {
        return configuration == rhs.configuration;
    }

    inline bool operator!=(const GamePosition &rhs) const {
        return !((*this)==rhs);
    }

    struct hasher {
        std::size_t operator()(const GamePosition &pos) const {
            return (uintptr_t)pos.configuration;
        }
    };
};
}}
#endif // POSITION_H
