#include "DistCZeroFPA.h"
#include "assert.h"
#include <iostream>
#include <algorithm>
#include "../../stopwatch.h"

#define STATS true
#define STATS_TIME false
#define HALT_ON false
#define AF_FIX_ON true

using namespace SearchStrategy;
using namespace DependencyGraph;

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
        strategy->pushDependency(d);
    }
    c->dependency_set.clear();

    //request edge halting
    for (Edge *s : c->successors) {
        strategy->pushEdge(s);
    }
}

void Algorithm::DistCZeroFPA::explore(Configuration *c)
{
    if (c->assignment == UNKNOWN && (c->hasActiveDependencies() || c == v)) {
        c->assignment = ZERO;
        if (partition->ownerId(c) == comm->rank()) {
            if (c->successors.empty()) {
                stopwatch watch;
                if (STATS_TIME) { watch.start(); }
                graph->successors(c);
                if (STATS_TIME) {
                    watch.stop();
                    s_time_generator += watch.duration();
                }
            }
            if (c->successors.empty()) {
                finalAssign(c, CZERO);
            }
            for (Edge *s : c->successors) {
                strategy->pushEdge(s);
            }
            if (STATS) {
                s_explored += 1;
                s_total_succ += c->successors.size();
                for (Edge *s : c->successors) {
                    s_total_targets += s->targets.size();
                }
            }
        } else {
            Message m(comm->rank(), c->getDistance(), Message::REQUEST, nextMessageId(), c);
            sendMessage(partition->ownerId(c), m);
        }
    }
}

void Algorithm::DistCZeroFPA::halt(Configuration *c)
{
    if (!HALT_ON) assert(false);
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
        if (STATS) s_answers += 1;
        finalAssign(c, ONE);
    } else if (m->type == Message::ANSWER_ZERO) {
        if (STATS) s_answers += 1;
        finalAssign(c, CZERO);
    } else if (m->type == Message::REQUEST) {
        if (STATS) s_requests += 1;
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
                explore(c);
            }
        }
    } else if (m->type == Message::HALT) {
        if (STATS) s_halts += 1;
        m->configuration->updateInterest(m->sender, -1 * m->id);
        halt(c);
    } else assert(false);
}

void Algorithm::DistCZeroFPA::processHyperEdge(Edge *e)
{
    if (STATS) s_edges += 1;
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
            if (HALT_ON) {
                if (e->requested != nullptr) {
                    halt(e->requested);
                    e->requested = nullptr;
                }
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else {
            assert(lastUndecided != nullptr);
            if (AF_FIX_ON && !strategy->available()) {
                for (auto t : e->targets) {
                    if (!t->isDone()) {
                        addDependency(e, t);
                        explore(t);
                    }
                }
            } else {
                e->requested = lastUndecided;
                addDependency(e, lastUndecided);
                explore(lastUndecided);
            }
        }
        e->processed = true;
    } else if (HALT_ON && e->requested != nullptr) {   //halt!
        e->processed = false;
        halt(e->requested);
        e->requested = nullptr;
    }
}

void Algorithm::DistCZeroFPA::processNegationEdge(Edge *e)
{
    if (STATS) s_negation_edges += 1;
    if(e->source->assignment == ZERO && !e->is_deleted){
        assert(e->targets.size() == 1);
        Configuration *target = e->targets.front();

        if (target->assignment == ONE) {
            e->source->removeSuccessor(e);
            if (HALT_ON) {
                if (e->requested != nullptr) {
                    //technically this shouldn't happen - maybe get rid of it?
                    halt(e->requested);
                    e->requested = nullptr;
                }
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else if (target->assignment == CZERO || (target->assignment == ZERO && e->processed)) {
            finalAssign(e->source, ONE);
        } else {
            e->requested = target;
            addDependency(e, target);
            explore(target);
        }
        e->processed = true;
    } else if (HALT_ON && e->requested != nullptr) {
        e->processed = false;
        halt(e->requested);
        e->requested = nullptr;
    }
}

bool Algorithm::DistCZeroFPA::terminationDetection()
{
    int receiver = (comm->rank() + 1) % comm->size();
    int source = (comm->rank() - 1 + comm->size()) % comm->size();
    std::pair<int, Token> t = comm->recvToken(source);
    if (t.first >= 0) { //we actually got a token
        if (STATS) s_tokens += 1;
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

    target->setDistance(std::max(sDist, tDist));
    target->dependency_set.push_back(e);
}


bool Algorithm::DistCZeroFPA::search(
        BasicDependencyGraph &t_graph, iDistributedSearchStrategy &t_strategy,
        Communicator &communicator, Algorithm::PartitionFunction &partition_function
) {
    this->partition = &partition_function;
    this->comm = &communicator;
    this->graph = &t_graph;
    this->strategy = &t_strategy;
    this->v = graph->initialConfiguration();

    termination_flag = FLAG_DIRTY;
    if (partition->ownerId(v) == comm->rank()) {
        explore(v);
    }

    int canPick = 0;

    stopwatch watch;

    int messages = 0;
    while (canPick >= 0) {

        if (STATS_TIME) watch.start();

        SearchStrategy::TaskType type;
        do {

            if (STATS_TIME) {
                watch.stop();
                s_time_idle += watch.duration();
            }

            if (STATS_TIME) watch.start();
            std::pair<int, Message> message = comm->recvMessage();
            while (message.first >= 0) {
                messages += 1;
                strategy->pushMessage(message.second);
                message = comm->recvMessage();
            }
            if (STATS_TIME) {
                watch.stop();
                s_time_in_receive += watch.duration();
            }

            if (STATS_TIME) watch.start();
            Edge *e;
            Message m;
            type = strategy->pickTask(e, m);
            if (type == TaskType::EDGE) {
                termination_flag = FLAG_DIRTY;
                if (e->is_negated) {
                    processNegationEdge(e);
                } else {
                    processHyperEdge(e);
                }
            } else if (type == TaskType::MESSAGE) {
                termination_flag = FLAG_DIRTY;
                processMessage(&m);
            } else {
                //this is ok if we are waiting for termination detection
            }

            if (STATS_TIME) {
                watch.stop();
                s_time_in_processing += watch.duration();
            }

            if (STATS_TIME) {
                watch.start();
            }
            if (v->isDone()) break;
        } while (!((type == TaskType::EMPTY || type == TaskType::UNAVAILABLE) && terminationDetection()));

        if (STATS_TIME) {
            watch.stop();
            s_time_idle += watch.duration();
        }

        if (v->isDone()) break;

        canPick = -1;
        if (!strategy->empty()) {
            canPick = (int) strategy->maxDistance();
        }

        if (STATS_TIME) s_negation_pick_rounds += 1;

        comm->computeMax(canPick);

        //notify search strategy
        if (canPick >= 0) {
            strategy->releaseNegationEdges(canPick);
        }
    }

    if (STATS) {
        int printing = 0;
        while (printing < comm->size()) {
            if (printing == comm->rank()) {
                std::cout << "[Worker " << comm->rank() << " printing stats]" << std::endl;
                std::cout << "[Processed edges] " << s_edges << std::endl;
                std::cout << "[Processed negation edges] " << s_negation_edges << std::endl;
                std::cout << "[Negation pick rounds] " << s_negation_pick_rounds << std::endl;
                std::cout << "[Requests received] " << s_requests << std::endl;
                std::cout << "[Answers received] " << s_answers << std::endl;
                std::cout << "[Halts received] " << s_halts << std::endl;
                std::cout << "[Tokens received] " << s_tokens << std::endl;
                std::cout << "[Configurations explored] " << s_explored << std::endl;
                std::cout << "[Avr. succ edges per configuration] " << (((double) s_total_succ) / ((double) s_total_targets)) << std::endl;
                std::cout << "[Avr. targets per edge] " << (((double) s_total_targets) / ((double) s_total_succ)) << std::endl;
                std::cout << "[Time spent receiving messages] " << s_time_in_receive << std::endl;
                std::cout << "[Time spent processing edges/messages] " << s_time_in_processing << std::endl;
                std::cout << "[Time idle] " << s_time_idle << std::endl;
                std::cout << "[Time spent in generator] " << s_time_generator << std::endl;
            }
            printing += 1;
            comm->computeMax(printing);
        }
    }

    return (v->assignment == ONE) ? true : false;
}

