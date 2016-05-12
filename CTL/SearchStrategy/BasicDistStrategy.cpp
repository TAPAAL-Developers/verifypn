#include "BasicDistStrategy.h"

namespace SearchStrategy {

BasicDistStrategy::BasicDistStrategy()
{

}

bool BasicDistStrategy::empty() const
{
    //std::cout << "Edges: " << edges.size() << " Mess: " << messages.size() << " Neg: " << negation_edges.size();
    return edges.empty() && messages.empty() && negation_edges.empty();
}

unsigned int BasicDistStrategy::maxDistance() const
{
    return negation_edges.maxDistance();
}

void BasicDistStrategy::pushEdge(DependencyGraph::Edge *edge)
{
    if (edge->is_negated) {
        //std::cout << "push negation" << std::endl;
        //edge->source->printConfiguration();
        negation_edges.push(edge);
    } else {
        //std::cout << "push: " << edge->source->is_negated << std::endl;
        //edge->source->printConfiguration();
        edges.push(edge);
    }
}

void BasicDistStrategy::pushMessage(Message &message)
{
    messages.push(message);
}

void BasicDistStrategy::releaseNegationEdges(int dist)
{
    negation_edges.releaseNegationEdges(dist);
}

TaskType BasicDistStrategy::pickTask(DependencyGraph::Edge *&edge, Message &message)
{
    if (!messages.empty()) {
        //std::cout << "pick" << std::endl;
        message = messages.top();
        messages.pop();
        return TaskType::MESSAGE;
    } else if (!edges.empty()) {
        //std::cout << "pick" << std::endl;
        edge = edges.top();
        //edge->source->printConfiguration();
        edges.pop();
        return TaskType::EDGE;
    } else if (negation_edges.pop(edge)) {
        //std::cout << "pick" << std::endl;
        //edge->source->printConfiguration();
        return TaskType::EDGE;
    } else if (!negation_edges.empty()) {
        return TaskType::UNAVAILABLE;
    } else {
        return TaskType::EMPTY;
    }
}

}
