#ifndef IGAME_H
#define IGAME_H

#include "Position.h"
#include "../DependencyGraph/Edge.h"
#include "../DependencyGraph/Configuration.h"

namespace DependencyGraph {

namespace Game {

class iPlayer {
public:
    virtual std::string PlayerName() =0;
    virtual DependencyGraph::Edge *Turn(const Configuration::container_type &edges) =0;
    virtual DependencyGraph::Configuration *Turn(const Edge::container_type &configurations) =0;
    virtual void AnnounceStart(GamePosition startPosition, const iPlayer *attacker, const iPlayer *defender) =0;
    virtual void AnnounceNextPosition(GamePosition nextPosition) =0;
    virtual void AnnounceEdgePicked(Edge *edge) =0;
    virtual void AnnounceConfigurationPicked(Configuration *configuration) =0;
    virtual void AnnounceEnd(GamePosition finalPosition, iPlayer *winner, iPlayer *looser) =0;
};
}
}

#endif // IGAME_H
