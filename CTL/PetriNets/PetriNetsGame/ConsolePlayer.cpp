#include "ConsolePlayer.h"
#include <iostream>

std::string PetriNets::ConsolePlayer::Rules() const
{
    return std::string( "These are the rules:\n"
                        "- Rule 1. You can never win\n"
                        "- Rule 2. You play as the attacker, and the game will defend the claim.\n"
                        "- Rule 3. The game will present you with your options,"
                        "-         and you can then select any of these choices.\n"
                        "-         Whatever you choose, you will still loose. \n"
                        "-         When you have lost, the trace will be displayed,"
                        "-         and this should convince you of your defeat. \n"
                        );
}

DependencyGraph::Edge *PetriNets::ConsolePlayer::Turn(const DependencyGraph::Configuration::container_type &edges)
{
    return *getInput(edges.begin(), edges.end());
}

DependencyGraph::Configuration *PetriNets::ConsolePlayer::Turn(const DependencyGraph::Edge::container_type &configurations)
{
    return *getInput(configurations.begin(), configurations.end());
}

void PetriNets::ConsolePlayer::AnnounceStart(DependencyGraph::Game::GamePosition startPosition,
                                             const DependencyGraph::Game::iPlayer *attacker,
                                             const DependencyGraph::Game::iPlayer *defender)
{
    std::cout << Rules() << std::endl;
}

void PetriNets::ConsolePlayer::AnnounceNextPosition(DependencyGraph::Game::GamePosition nextPosition)
{
    currentPosition = nextPosition;
}

void PetriNets::ConsolePlayer::AnnounceEdgePicked(DependencyGraph::Edge *edge)
{
    pickedEdge = edge;

    std::cout << "Defender picks edge: " << std::endl
              << edge->toString()
              << std::endl << std::endl << std::endl;
}

void PetriNets::ConsolePlayer::AnnounceConfigurationPicked(DependencyGraph::Configuration *configuration)
{
    pickedConfig = configuration;

    std::cout << "Defender picks configuration:" << std::endl
              << configuration->toString()
              << std::endl << std::endl << std::endl;
}

void PetriNets::ConsolePlayer::AnnounceEnd(DependencyGraph::Game::GamePosition finalPosition, DependencyGraph::Game::iPlayer *winner, DependencyGraph::Game::iPlayer *looser)
{
    currentPosition = finalPosition;

    std::cout << "Player " << winner->PlayerName() << " won" << std::endl
              << "Final position was: "
              << "Claim: " << finalPosition.claim << std::endl
              << finalPosition.configuration->toString() << std::endl;
}

template<class inputIterator>
inputIterator PetriNets::ConsolePlayer::getInput(inputIterator first, inputIterator last)
{
    std::cout<<"Please select an item from the list: "<<"\n"<<std::endl;

    inputIterator current = first;
    int count = 0;

    while(current != last){
        std::cout << "Option: " << ++count << std::endl;
        std::cout << (*current)->toString() << std::endl;
        current++;
    }

    std::cout << std::endl << std::endl << std::endl;

    int selected_edge;
    bool getInput = true;

    do {
        std::cin >> selected_edge;

        if(1 <= selected_edge && selected_edge <= count){
            getInput = false;
        }
        else {
            std::cout<<"Please provide an actual number from the list."<<std::endl;
        }

    } while (getInput);

    return first + (selected_edge - 1);
}
