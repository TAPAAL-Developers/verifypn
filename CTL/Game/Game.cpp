#include "Game.h"
#include <assert.h>
#include <algorithm>

namespace DependencyGraph { namespace Game {

void Game::Play(GamePosition startPosition)
{
    assert(attacker != nullptr);
    assert(defender != nullptr);

    int round = 0;
    bool lastRound = false;
    positions.clear();
    rounds.clear();

    GamePosition currentPosition = startPosition;
    positions.insert(currentPosition);

    GameRound currentRound;
    currentRound.position = currentPosition;

    attacker->AnnounceStart(currentRound.position, attacker, defender);
    defender->AnnounceStart(currentRound.position, attacker, defender);

    do {
        currentRound = currentPosition.claim == 1 ? PlayRound(currentPosition, defender, attacker) :
                                                PlayRound(currentPosition, attacker, defender);
        currentRound.round = ++round;
        rounds.insert(rounds.end(), currentRound);
        lastRound = FinalRound(currentRound);

        currentPosition = GamePosition(currentRound.configuration,
                                   currentRound.edge->is_negated ? 1 - currentRound.position.claim :
                                                                   currentRound.position.claim);

        infiniteGame = !positions.insert(currentPosition).second;

        if(!lastRound){
            attacker->AnnounceNextPosition(currentPosition);
            defender->AnnounceNextPosition(currentPosition);
        }

    }while(!lastRound);

    winner = Winner(currentRound);
    looser = winner == defender ? attacker : defender;

    attacker->AnnounceEnd(currentRound.position, winner, looser);
    defender->AnnounceEnd(currentRound.position, winner, looser);
}

GameRound Game::PlayRound(const GamePosition &pos, iPlayer *edgePicker, iPlayer *configPicker)
{
    GameRound round = GameRound();
    round.position = pos;
    Configuration &configuration = *pos.configuration;

    if(!configuration.successors.empty() || !configuration.deleted_successors.empty()){

        round.edge = edgePicker->Turn(GetAllSuccessors(round.position.configuration));
        configPicker->AnnounceEdgePicked(round.edge);

        if(!round.edge->targets.empty()){

            round.configuration = configPicker->Turn(round.edge->targets);
            edgePicker->AnnounceConfigurationPicked(round.configuration);
        }
    }

    return round;
}

bool Game::FinalRound(GameRound &round) const
{
    if(round.edge == nullptr){
        return true;
    }
    //If no configuration can be picked, game stops
    else if (round.configuration == nullptr){
        return true;
    }
    else {
        GamePosition newPosition = GamePosition(round.configuration,
                                                round.edge->is_negated ? 1 - round.position.claim :
                                                                         round.position.claim);

        return positions.find(newPosition) != positions.end();
    }
}

iPlayer *Game::Winner(const GameRound &finalRound)
{
    const GamePosition &finalPosition = finalRound.position;
    iPlayer *winner = nullptr;

    if(finalRound.edge == nullptr){
        if(finalPosition.claim == 1)
            winner = attacker;
        else
            winner = defender;
    }
    else if(finalRound.configuration == nullptr){
        if(finalPosition.claim == 0)
            winner = attacker;
        else
            winner = defender;
    }
    else if(infiniteGame){
        if(finalPosition.claim == 1)
            winner = attacker;
        else
            winner = defender;
    }

    return winner;
}

Configuration::container_type Game::Game::GetAllSuccessors(Configuration *&c)
{
    Configuration::container_type edges;
    edges.insert(edges.end(), c->successors.begin(), c->successors.end());
    edges.insert(edges.end(), c->deleted_successors.begin(), c->deleted_successors.end());

    return edges;
}
}//Game
}//DependencyGraph
