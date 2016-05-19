#include "ComputerPlayer.h"
#include <assert.h>

namespace PetriNets {

bool ComputerPlayer::Safe(Edge *e) const
{
    for(Configuration *c : e->targets){
        if(c->assignment != ONE)
            return false;
    }
    return true;
}

Configuration *ComputerPlayer::Turn(const Edge::container_type &configurations)
{
    //First we look for CZero assignment
    for(Configuration *c : configurations){
        if(c->assignment == CZERO) {
            return c;
        }
    }

    //Then we settle for Zero
    for(Configuration *c : configurations){
        if(c->assignment == ZERO)
            return c;
    }

    //And lastly, just return anything
    return *configurations.begin();
}

Edge *ComputerPlayer::Turn(const Configuration::container_type &edges)
{
    for(Edge *e : edges){
        if(Safe(e) xor e->is_negated) {
            return e;
        }
    }

    return *edges.begin();
}

void ComputerPlayer::AnnounceStart(Game::GamePosition startPosition, const Game::iPlayer *attacker, const Game::iPlayer *defender)
{
    currentPosition = startPosition;
}

void ComputerPlayer::AnnounceNextPosition(Game::GamePosition nextPosition)
{
    currentPosition = nextPosition;
}

void ComputerPlayer::AnnounceEdgePicked(Edge *edge)
{
    pickedEdge = edge;
}

void ComputerPlayer::AnnounceConfigurationPicked(Configuration *configuration)
{
    pickedConfig = configuration;
}

void ComputerPlayer::AnnounceEnd(Game::GamePosition finalPosition, Game::iPlayer *winner, Game::iPlayer *looser)
{
    currentPosition = finalPosition;
    pickedConfig = nullptr;
    pickedConfig = nullptr;
}

}
