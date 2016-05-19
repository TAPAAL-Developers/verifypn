#ifndef COMPUTERPLAYER_H
#define COMPUTERPLAYER_H

#include "../../Game/iPlayer.h"
#include "../../DependencyGraph/BasicDependencyGraph.h"
#include "../../DependencyGraph/Configuration.h"
#include "../../DependencyGraph/Edge.h"

namespace PetriNets {

using namespace DependencyGraph;

class ComputerPlayer : public Game::iPlayer
{
    Game::GamePosition currentPosition;
    Edge *pickedEdge;
    Configuration *pickedConfig;

    bool Safe(Edge *e) const;

    // iPlayer interface
public:
    virtual std::string PlayerName() { return std::string("HAL v0.1");}
    virtual Edge *Turn(const Configuration::container_type &edges) override;
    virtual Configuration *Turn(const Edge::container_type &configurations) override;
    virtual void AnnounceStart(Game::GamePosition startPosition, const Game::iPlayer *attacker, const Game::iPlayer *defender) override;
    virtual void AnnounceNextPosition(Game::GamePosition nextPosition) override;
    virtual void AnnounceEdgePicked(Edge *edge) override;
    virtual void AnnounceConfigurationPicked(Configuration *configuration) override;
    virtual void AnnounceEnd(Game::GamePosition finalPosition, Game::iPlayer *winner, Game::iPlayer *looser) override;
};

}
#endif // COMPUTERPLAYER_H
