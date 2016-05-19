#ifndef PETRIGAME_H
#define PETRIGAME_H

#include "../../Game/iPlayer.h"
#include "../../Game/Game.h"
#include "../PetriConfig.h"
#include "../../../PetriEngine/PetriNet.h"

namespace PetriNets {

using namespace DependencyGraph;

class PetriGame
{
protected:
    Game::iPlayer *player;
    PetriEngine::PetriNet *net;
public:
    PetriGame(PetriEngine::PetriNet *net, Game::iPlayer *player = nullptr) : net(net), player(player) {}
    virtual Game::iPlayer *Play(Configuration &initialConfiguration);
};
}
#endif // PETRIGAME_H
