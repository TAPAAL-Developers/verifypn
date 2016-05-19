#ifndef GAME_H
#define GAME_H

#include "iPlayer.h"
#include "Round.h"
#include "Position.h"
#include <vector>
#include <tr1/unordered_set>

namespace DependencyGraph { namespace Game {

class Game
{
    iPlayer *attacker = nullptr;
    iPlayer *defender = nullptr;
public:
    std::tr1::unordered_set<GamePosition, GamePosition::hasher> positions;
    std::vector<GameRound> rounds;


    iPlayer *winner = nullptr;
    iPlayer *looser = nullptr;
    bool infiniteGame = false;

    //Start match
    void Play(GamePosition startPosition);
    void Attacker(iPlayer *attacker) { this->attacker = attacker; }
    void Defender(iPlayer *defender) { this->defender = defender; }

protected:
    bool FinalRound(GameRound &round) const;
    iPlayer* Winner(const GameRound &finalRound);
    GameRound PlayRound(const GamePosition &pos, iPlayer *edgePicker, iPlayer *configPicker);
    Configuration::container_type GetAllSuccessors(Configuration *&c);
};
}}
#endif // GAME_H
