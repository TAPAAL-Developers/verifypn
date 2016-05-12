#include "DistCZeroFPA.h"
#include "assert.h"
#include <iostream>
#include <algorithm>

using namespace SearchStrategy;
using namespace DependencyGraph;

Algorithm::DistCZeroFPA::DistCZeroFPA(Algorithm::PartitionFunction *partition, Communicator *comm) :
    partition(partition), comm(comm)
{ }

void Algorithm::DistCZeroFPA::finalAssign(Configuration *c, Assignment value)
{
    c->assignment = value;
    Message::Type type = value == ONE ? Message::ANSWER_ONE : Message::ANSWER_ZERO;
    if(v == c && partition->ownerId(c) == comm->rank()) {
        //Notify everyone else that v is done, bypassing termination
        for (int i = 0; i < comm->size(); i++){
            if (i != comm->rank()) {
                Message m(comm->rank(), c->getDistance(), type, nextMessageId(), c);
                sendMessage(i, m);
            }
        }
    }

    for (std::pair<int, long> interest : c->interested) {
        if (interest.second > 0) {
            assert(partition->ownerId(c) == comm->rank());
            Message m(comm->rank(), c->getDistance(), type, nextMessageId(), c);
            sendMessage(interest.first, m);
        }
    }
    for(Edge *d : c->dependency_set) {
        strategy->pushEdge(d);
    }
    c->dependency_set.clear();

    //request edge halting
    for (Edge *s : c->successors) {
        strategy->pushEdge(s);
    }
}

void Algorithm::DistCZeroFPA::explore(Configuration *c)
{   
    if (comm->rank() == 1) {
        c->printConfiguration();
    }
    if (c->assignment == UNKNOWN && (c->hasActiveDependencies() || c == v)) {
        c->assignment = ZERO;
        if (partition->ownerId(c) == comm->rank()) {
            if (c->successors.empty()) {
                graph->successors(c);
            }
            if (c->successors.empty()) {
                finalAssign(c, CZERO);
            }
            for (Edge *s : c->successors) {
                strategy->pushEdge(s);
            }
        } else {
            Message m(comm->rank(), c->getDistance(), Message::REQUEST, nextMessageId(), c);
            sendMessage(partition->ownerId(c), m);
        }
    }
}

void Algorithm::DistCZeroFPA::halt(Configuration *c)
{
    assert(false);
    if (!(c->hasActiveDependencies() || c == v || c->isDone())) {
        c->assignment = UNKNOWN;
        if (partition->ownerId(c) == comm->rank()) {
            for (Edge *s : c->successors) {
                strategy->pushEdge(s);
            }
        } else {
            Message m(comm->rank(), c->getDistance(), Message::HALT, nextMessageId(), c);
            sendMessage(partition->ownerId(c), m);
        }
    }
}


void Algorithm::DistCZeroFPA::processMessage(Message *m)
{
    message_counter -= 1;
    Configuration *c = m->configuration;

    if (m->type == Message::ANSWER_ONE) {
        finalAssign(c, ONE);
    } else if (m->type == Message::ANSWER_ZERO) {
        finalAssign(c, CZERO);
    } else if (m->type == Message::REQUEST) {
        assert(partition->ownerId(m->configuration) == comm->rank());
        if (c->isDone()) {
            Message::Type t = c->assignment == ONE ? Message::ANSWER_ONE : Message::ANSWER_ZERO;
            Message out(comm->rank(), c->getDistance(), t, nextMessageId(), c);
            sendMessage(m->sender, out);
        } else {
            //update the distance
            c->setDistance(std::max(c->getDistance(), m->distance));
            c->updateInterest(m->sender, m->id);
            if (c->hasActiveDependencies() && c->assignment == UNKNOWN) {                
                /*if (comm->rank() == 1) {
                    std::cout << comm->rank() << " Explore from message " << std::endl;
                }*/
                explore(c);
            }
        }
    } else if (m->type == Message::HALT) {
        m->configuration->updateInterest(m->sender, -1 * m->id);
        halt(c);
    } else assert(false);
}

void Algorithm::DistCZeroFPA::processHyperEdge(Edge *e)
{
    if(e->source->assignment == ZERO && !e->is_deleted) {

        bool allOne = true;
        bool hasCZero = false;
        Configuration *lastUndecided = nullptr;

        for (DependencyGraph::Configuration *c : e->targets) {
            if (c->assignment == CZERO) {
                hasCZero = true;
            }
            if (c->assignment != ONE) {
                allOne = false;
            }
            if (!c->isDone()) {
                lastUndecided = c;
            }
        }

        if (allOne) {
            finalAssign(e->source, ONE);
        } else if (hasCZero) {
            e->source->removeSuccessor(e);
            if (e->requested != nullptr) {
                //technically this shouldn't happen - maybe get rid of it?
                //halt(e->requested);
                //e->requested = nullptr;
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else {
            assert(lastUndecided != nullptr);
            e->requested = lastUndecided;
            addDependency(e, lastUndecided);
            explore(lastUndecided);
        }
    } else if (e->requested != nullptr) {   //halt!
        //halt(e->requested);
        //e->requested = nullptr;
    }
}

void Algorithm::DistCZeroFPA::processNegationEdge(Edge *e)
{
    if(e->source->assignment == ZERO && !e->is_deleted){
        /*if (comm->rank() == 1) {
            std::cout << comm->rank() << " Process " << e << std::endl;
        }*/
        assert(e->targets.size() == 1);
        Configuration *target = e->targets.front();

        if (target->assignment == ONE) {
            e->source->removeSuccessor(e);
            if (e->requested != nullptr) {
                //technically this shouldn't happen - maybe get rid of it?
                //halt(e->requested);
                //e->requested = nullptr;
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else if ((target->assignment == CZERO || target->assignment == ZERO) && e->processed) {
            /*if (comm->rank() == 1) {
                std::cout << comm->rank() << " Assign one " << e << std::endl;
                target->printConfiguration();
            }*/
            finalAssign(e->source, ONE);
        } else if (target->assignment == UNKNOWN) {
            /*if (comm->rank() == 1) {
                std::cout << comm->rank() << " Explore negation" << e << std::endl;
            }*/
            e->requested = target;
            addDependency(e, target);
            strategy->pushEdge(e);      //repush the negation edge
            explore(target);
        } else {
            //In case someone discovered the assignment before us (from other negation edge f.e.)
            assert(target->assignment == ZERO);
            e->requested = target;
            addDependency(e, target);
            strategy->pushEdge(e);
        }
        e->processed = true;
    } else if (e->requested != nullptr) {
        //halt(e->requested);
        //e->requested = nullptr;
    }
}

bool Algorithm::DistCZeroFPA::terminationDetection()
{
    int receiver = (comm->rank() + 1) % comm->size();
    int source = (comm->rank() - 1 + comm->size()) % comm->size();
    std::pair<int, Token> t = comm->recvToken(source);
    if (t.first >= 0) { //we actually got a token
        Token token = t.second;
        if (comm->rank() == 0) {            
            if (token.flag == FLAG_CLEAN && token.messages == 0) {
                Token t(FLAG_TERMINATE, 0);
                if (receiver != 0) comm->sendToken(receiver, t);
                //this will initialize the next round
                waiting_for_token = false;
                termination_flag = FLAG_DIRTY;
                return true;
            } else {
                waiting_for_token = true;
                Token t(termination_flag, message_counter);
                comm->sendToken(receiver, t);
                termination_flag = FLAG_CLEAN;
            }
        } else {
            if (token.flag == FLAG_TERMINATE) {
                //notify next one
                if (receiver != 0) comm->sendToken(receiver, token);
                termination_flag = FLAG_DIRTY;
                return true;
            } else {
                Token t(token.flag == FLAG_DIRTY ? FLAG_DIRTY : termination_flag, token.messages + message_counter);
                comm->sendToken(receiver, t);
                termination_flag = FLAG_CLEAN;
            }
        }
    } else if (comm->rank() == 0 && !waiting_for_token) {
        //init first round
        waiting_for_token = true;
        Token t(termination_flag, message_counter);
        comm->sendToken(receiver, t);
        termination_flag = FLAG_CLEAN;
    }
    return false;
}

void Algorithm::DistCZeroFPA::sendMessage(int receiver, Message &m)
{
    message_counter += 1;
    comm->sendMessage(receiver, m);
}

void Algorithm::DistCZeroFPA::addDependency(Edge *e, Configuration *target)
{
    unsigned int sDist = e->is_negated ? e->source->getDistance() + 1 : e->source->getDistance();
    unsigned int tDist = target->getDistance();

    //std::cout << "Update dist: " << sDist << tDist << std::endl;
    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}

bool Algorithm::DistCZeroFPA::search(BasicDependencyGraph &t_graph, iDistributedSearchStrategy &t_strategy)
{
    this->graph = &t_graph;
    this->strategy = &t_strategy;
    this->v = graph->initialConfiguration();

    termination_flag = FLAG_DIRTY;
    std::cout << "Initial partition: " << partition->ownerId(v) << std::endl;
    if (partition->ownerId(v) == comm->rank()) {
        std::cout << "Exploring initial" << std::endl;
        explore(v);
    }

    int canPick = 0;
    int processed = 0;

    while (canPick >= 0) {
        SearchStrategy::TaskType type;
        do {


            std::pair<int, Message> message = comm->recvMessage();
            while (message.first >= 0) {
                strategy->pushMessage(message.second);
                message = comm->recvMessage();
            }

            Edge *e;
            Message m;
            type = strategy->pickTask(e, m);
            if (type == TaskType::EDGE) {
                termination_flag = FLAG_DIRTY;
                if (e->is_negated) {
                    //std::cout << "Process negation edge " << std::endl;
                    processed += 1;
                    processNegationEdge(e);
                } else {
                    processed += 1;
                    processHyperEdge(e);
                }
            } else if (type == TaskType::MESSAGE) {
                termination_flag = FLAG_DIRTY;
                processMessage(&m);
            } else {
                //this is ok if we are waiting for termination detection
            }

            if (v->isDone()) break;
        } while (!((type == TaskType::EMPTY || type == TaskType::UNAVAILABLE) && terminationDetection()));

        strategy->empty();

        if (v->isDone()) break;

        int candidate = -1;
        if (!strategy->empty()) {
            candidate = strategy->maxDistance();
        }

        //if we are not master, send proposal
        if (comm->rank() != 0) {
            std::cout << comm->rank() << "send distance" << std::endl;
            comm->sendDistance(0, candidate);
        } else {
            //if we are master, collect proposals and compute minimum
            for (int i=1; i<comm->size(); i++) {
                std::pair<int, int> proposal = comm->recvDistance();
                while (proposal.first < 0) {
                    proposal = comm->recvDistance();
                }
                std::cout << "got distance" << std::endl;
                if (proposal.second > candidate) {
                    candidate = proposal.second;
                }
            }
        }

        //broadcast results of vote to everyone
        canPick = candidate;
        std::cout << comm->rank() << " send broadcast " << canPick << std::endl;
        comm->broadcastDistance(0, canPick);

        //notify search strategy
        if (canPick >= 0) {
            strategy->releaseNegationEdges(canPick);
        }
    }

    std::cout << "Processed: " << processed << " " << strategy->empty() << std::endl;
    std::cout << "Empty: " << strategy->empty() << std::endl;

    return (v->assignment == ONE) ? true : false;
}

