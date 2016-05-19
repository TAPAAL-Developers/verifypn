#ifndef CONSOLEPLAYER_H
#define CONSOLEPLAYER_H

#include "../../Game/iPlayer.h"
#include "../PetriConfig.h"
#include "../Marking.h"

namespace PetriNets {

using namespace DependencyGraph;

class ConsolePlayer : public Game::iPlayer
{
public:
    PetriEngine::PetriNet *net = nullptr;
    Game::GamePosition currentPosition;
    Edge* pickedEdge = nullptr;
    Configuration * pickedConfig = nullptr;

    ConsolePlayer(PetriEngine::PetriNet *net = nullptr) : net(net) {}

    std::string Rules() const;


    // iPlayer interface
public:
    virtual std::string PlayerName() override { return std::string("ConsolePlayer"); }
    virtual Edge *Turn(const Configuration::container_type &edges) override;
    virtual Configuration *Turn(const Edge::container_type &configurations) override;
    virtual void AnnounceStart(Game::GamePosition startPosition,const Game::iPlayer *attacker, const Game::iPlayer *defender) override;
    virtual void AnnounceNextPosition(Game::GamePosition nextPosition) override;
    virtual void AnnounceEdgePicked(Edge *edge) override;
    virtual void AnnounceConfigurationPicked(Configuration *configuration) override;
    virtual void AnnounceEnd(Game::GamePosition finalPosition, Game::iPlayer *winner, Game::iPlayer *looser) override;

private:
    template<class inputIterator>
    inputIterator getInput(inputIterator first, inputIterator last);
};
}
#endif // CONSOLEPLAYER_H
