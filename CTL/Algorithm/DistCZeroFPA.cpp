/*#include "DistCZeroFPA.h"
#include "PartitionFunction.h"
//#include "mpi.h"
#include "UniformPartitionFunction.h"
#include "DistributiveSumPartitionFunction.h"
#include "DistributionRatioPartitionFunction.h"
#include <sched.h>

///Debugging area begin
//Debugging information types
#define LOGGING false            //Disables logging
#define PRINTING false           //Disables printing
#define STATUS_PRINT false
//#define NDEBUG                  //If commented, excludes asserts

//Debugging areas: True or false disables debuggin in that area
#define SEARCH false
#define PROCESS_HYPER false
#define PROCESS_NEGATION false
#define PROCESS_MESSAGE false
#define EXPLORE false
#define ASSIGN false
#define TRY_RECV_LOG false
#define NEG_DETECT false
#define NEG_TERMINATION false
#define NEG_VOTE false

//Generic debugging functions
#define BOOL(b) b ? "true" : "false"
#define ASSIGNMENT(a) a == ONE ? "ONE" : a == UNKNOWN ? "UNKOWN" : a == ZERO ? "ZERO" : "CZERO"

//Note: If disable, output is undefined. #if LOGGING || PRINTING
#define OUTPUT(...) { \
    std::stringstream prefix; \
    prefix << ":::::::::::::::::::::: " << "Wrk " << WORKER_ID << " - " << __func__ << __LINE__ << " ::::::::::::::::::::::" << std::endl;\
    output(prefix.str(), __VA_ARGS__);\
    }
//#else
//#endif

#if STATUS_PRINT
#define STATUS() printf("Wrk %lu - %s %d\n", WORKER_ID, __func__, __LINE__)
#else
#define STATUS()
#endif

#if STATUS_PRINT
#define EXIT_STATUS() printf("Wrk %lu - %s %d : EXITING\n", WORKER_ID, __func__, __LINE__)
#else
#define EXIT_STATUS()
#endif


///Debugging area end

namespace ctl{

DistCZeroFPA::DistCZeroFPA() : DistFixedPointAlgorithm() {

}

void DistCZeroFPA::process_message(int* data, int sender) {
    int timestamp = data[0];
    Assignment value = (Assignment) data[1];


    Configuration *c = graph->deserialize(data + 2);
    int owner = _partition->ownerId(c);

    if(PROCESS_MESSAGE) OUTPUT("Sender: ", sender, " Value: ", ASSIGNMENT(value), "\n", c->to_string());

    //Answer from request
    if(owner != WORKER_ID){
        assert(!c->isDone() || c->assignment == value);
        finalAssign(*c, value);
    }
    else {
        if (value == ZERO) {    //request
            if (c->assignment == ONE || c->assignment == CZERO) {
                //Configuration is done, just ping back the sender
                wrapper.SendValue(sender, *c, c->assignment);
            } else {
                c->updateInterest(sender, timestamp);
                if (c->hasActiveDependencies() && c->assignment == UNKNOWN) {
                    explore(*c);
                }
            }
        } else if (value == UNKNOWN) {  //halt
            c->updateInterest(sender, -1 * timestamp);
            halt(c);
        }
    }
}

int DistCZeroFPA::try_recv_messages(){
    int tmp_received = 0;
    while (true) {
        MPI_Status status;
        int hasMessage = 0;
        wrapper.MPIGetStatus(&hasMessage, &status, MPI_STATE_TAG);

        if (!hasMessage) {
            break;
        }
        
        int message_size = 0;        
        wrapper.MPIGetCount(&status, &message_size);
        int *buffer = (int*) malloc(sizeof(int) * message_size);

        wrapper.MPIGetMessage(buffer, message_size, status);

        process_message(buffer, status.MPI_SOURCE);        
        wrapper.received_counter += 1;
        free(buffer);
        
        tmp_received++;
    }
    return tmp_received;
}

bool DistCZeroFPA::terminationDetection(){    
    int receiver = (WORKER_ID + 1) % NUMBER_OF_WORKERS;
    int source = (WORKER_ID - 1 + NUMBER_OF_WORKERS) % NUMBER_OF_WORKERS;
    MPI_Status status;
    int hasToken = 0;
    MPI_Test(&pendingToken, &hasToken, &status);
    if (hasToken) {
        //tokenBuf now contains the token
        if (WORKER_ID == 0) {
            if (tokenBuf[0] == 0 && tokenBuf[1] == FLAG_CLEAN) {
                //notify other workers
                if (receiver != 0) wrapper.SendToken(receiver, 0, FLAG_TERMINATE);
                //init next termination round
                waiting_for_token = false;
                termination_flag = FLAG_DIRTY;
                MPI_Irecv(&tokenBuf, 2, MPI_INT, source, MPI_TERMINATE_TAG, MPI_COMM_WORLD, &pendingToken);
                return true;
            } else {
                counter += 1;
                waiting_for_token = true;
                wrapper.SendToken(receiver, (wrapper.exported_counter - wrapper.received_counter), termination_flag);
                termination_flag = FLAG_CLEAN;
            }
        } else {
            if (tokenBuf[1] == FLAG_TERMINATE) {
                //notify next worker
                if (receiver != 0) wrapper.SendToken(receiver, 0, FLAG_TERMINATE);
                //init next terminatin round
                termination_flag = FLAG_DIRTY;
                MPI_Irecv(&tokenBuf, 2, MPI_INT, source, MPI_TERMINATE_TAG, MPI_COMM_WORLD, &pendingToken);
                return true;
            }
            int flag = termination_flag;
            if (tokenBuf[1] == FLAG_DIRTY) {
                flag = FLAG_DIRTY;
            }            
            wrapper.SendToken(receiver, tokenBuf[0] + (wrapper.exported_counter - wrapper.received_counter), flag);
            termination_flag = FLAG_CLEAN;
        }
        //reset pendingToken
        MPI_Irecv(&tokenBuf, 2, MPI_INT, source, MPI_TERMINATE_TAG, MPI_COMM_WORLD, &pendingToken);
    } else if (!waiting_for_token && WORKER_ID == 0) {
        //initialize the first tarmination round
        waiting_for_token = true;
        wrapper.SendToken(receiver, (wrapper.exported_counter - wrapper.received_counter), termination_flag);
        termination_flag = FLAG_CLEAN;
    }
    return false;
}


/// TODO: This might need fixing in the future.
/// For now it seems fine
void DistCZeroFPA::finalAssign(Configuration &c, Assignment assignedValue){
    c.assignment = assignedValue;
    if(v == &c && _partition->ownerId(&c) == WORKER_ID) {
        //Notify everyone else that v is done, bypassing termination
        for (int i = 0; i < NUMBER_OF_WORKERS; i++){
            if (i != WORKER_ID) {                
                wrapper.SendValue(i, c, assignedValue);
            }
        }
    }

    for (int i=0; i<c.Interested.size(); i++) {
        if (c.InterestedTimestamp[i] > 0) {
            wrapper.SendValue(c.Interested[i], c, assignedValue);
        }
    }
    for(Edge *de : c.DependencySet) {
        W->push_dependency(de);
    }
    c.DependencySet.clear();

    //request edge halting
    for (Edge *e : c.Successors) {
        //For some reason, this is seems to be slower: if (!e->requested.empty()) {
        W->push(e);
    }
}



void DistCZeroFPA::processHyperEdge(Edge * e){    
    if(e->source->assignment == ZERO) {
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        Configuration * lastUnknown = NULL;
        Configuration * anotherUnknown = NULL;
        bool czero = false;

        for(auto c : e->targets){
            if (c->assignment == ONE) {
                targetONEassignments++;
            }
            else if (c->assignment == ZERO) {
                targetZEROassignments++;
            }
            else if (c->assignment == CZERO){
                czero = true;
                break;
            }
            else if(c-> assignment == UNKNOWN){
                anotherUnknown = lastUnknown;
                lastUnknown = c;
            }

        }

        if(e->targets.size() == targetONEassignments){
            finalAssign(*e->source, ONE);
        }
        else if(czero){
            deleteEdge(e);
            for (Configuration * target : e->requested) {
                target->DependencySet.remove(e);
                halt(target);
            }
            e->requested.clear();
            if(e->source->Successors.size() == 0) {
                finalAssign(*e->source, CZERO);
            }
        }
        else if(targetZEROassignments > 0){
            for(Configuration *tc : e->targets){
                if(tc->assignment == ZERO){
                    tc->DependencySet.push_back(e);
                    e->requested.push_back(tc);
                }
            }
        }
        else if(lastUnknown != NULL){
            if (anotherUnknown != NULL && W->empty()) {
                //Overall not that faster
                e->requested.push_back(anotherUnknown);
                anotherUnknown->DependencySet.push_back(e);
                explore(*anotherUnknown);
            }
            e->requested.push_back(lastUnknown);
            lastUnknown->DependencySet.push_back(e);
            explore(*lastUnknown);
        }
                    
        e->processed = true;
    } else {    //we should halt!
        for (Configuration * target : e->requested) {
            target->DependencySet.remove(e);
            halt(target);
        }
        e->requested.clear();
    }
}

void DistCZeroFPA::processNegationEdge(Edge * e){
    if(e->source->assignment == ZERO){
        //TODO: Refactor this method to reflect that there is just one target
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        Configuration * lastUnknown = NULL;
        bool czero = false;

        for(auto c : e->targets){
            if (c->assignment == ONE) {
                targetONEassignments++;
            }
            else if (c->assignment == ZERO) {
                targetZEROassignments++;
            }
            else if (c->assignment == CZERO){
                czero = true;
                break;
            }
            else if(c->assignment == UNKNOWN){
                lastUnknown = c;
            }            
        }
        if (e->isDeleted){}
        else if(e->targets.size() == targetONEassignments){
            finalAssign(*e->source, CZERO);
            e->source->removeSuccessor(e);
        }
        else if(czero){
            //we assume there is just one target so we don't need to halt
            finalAssign(*e->source, ONE);
        }
        else if(targetZEROassignments > 0 && e->processed){
            finalAssign(*e->source, ONE);
        }
        else if(lastUnknown != NULL){
            e->requested.push_back(lastUnknown);
            lastUnknown->DependencySet.push_back(e);
            explore(*lastUnknown);            
        }
                    
        e->processed = true;
    } else {
        e->processed = false;
        for (Configuration * target : e->requested) {
            target->DependencySet.remove(e);
            if (!target->hasActiveDependencies()) {
                halt(target);
            }
        }
        e->requested.clear();
    }
}

void DistCZeroFPA::explore(Configuration& v){
    v.assignment = ZERO;
    if(_partition->ownerId(&v) == WORKER_ID){
        if (v.Successors.empty()) {
            graph->successors(v);
        }
        if(v.Successors.empty()){
            finalAssign(v, CZERO);
        }
        else{
            for(Edge *succ : v.Successors){
                W->push(succ);
                if(succ->source->IsNegated){
                    N.push(succ);
                }
            }
        }
    }
    else {
        wrapper.Request(_partition->ownerId(&v), v, ZERO);
    }
}

void DistCZeroFPA::halt(Configuration *c)
{
    if (!c->hasActiveDependencies() && c != v) {
        if (!c->isDone()) {
            c->assignment = UNKNOWN;
            if (_partition->ownerId(c) == WORKER_ID) {
                for (Edge *e : c->Successors) {
                    W->push(e);
                }
            } else {
                wrapper.Request(_partition->ownerId(c), *c, UNKNOWN);
            }
        }
    }
}

//Implemented to be consistent with pseudo code
//And to handle possible future change
void DistCZeroFPA::deleteEdge(Edge *e)
{
    e->source->removeSuccessor(e);
}

bool DistCZeroFPA::preprocessQuery(CTLTree *f)
{
    bool isTemporal = f->quantifier == A || f->quantifier == E;
    if (f->quantifier == AND ||
            f->quantifier == OR ||
            (f->quantifier == A && f->path == U) ||
            (f->quantifier == E && f->path == U)) {
        //operand order guarantees subquery will be preprocessed
        isTemporal = preprocessQuery(f->second) || isTemporal;
        isTemporal = preprocessQuery(f->first) || isTemporal;
    }
    if (f->quantifier == NEG ||
            (f->path == G && (f->quantifier == A || f->quantifier == E)) ||
            (f->path == X && (f->quantifier == A || f->quantifier == E)) ||
            (f->path == F && (f->quantifier == A || f->quantifier == E))) {
        isTemporal = preprocessQuery(f->first) || isTemporal;
    }
    f->isTemporal = isTemporal;
    return isTemporal;
}

bool DistCZeroFPA::search(DependencyGraph &t_graph, AbstractSearchStrategy &strategy) {
    STATUS();
    this->graph = &t_graph;
    this->W = &strategy;
    this->v = &t_graph.initialConfiguration();    
    preprocessQuery(v->query);
    _partition = new ctl::UniformPartitionFunction(NUMBER_OF_WORKERS);
    STATUS();
    MPI_Barrier(MPI_COMM_WORLD);
#if LOGGING
    std::cout << "Logging enabled" << std::endl;
    std::stringstream filename;
    filename << "worker_" << WORKER_ID << ".log";
    std::cout << filename.str() << std::endl;
    ostream.open(filename.str(), std::ios_base::out | std::ios_base::trunc);
    assert(ostream.is_open());
#endif

    STATUS();
    MPI_Barrier(MPI_COMM_WORLD);
    termination_flag = FLAG_DIRTY;
    if(_partition->ownerId(v) == WORKER_ID){
        explore(*v);
    }

    STATUS();
    MPI_Barrier(MPI_COMM_WORLD);

    //init pendingToken
    int source = (WORKER_ID - 1 + NUMBER_OF_WORKERS) % NUMBER_OF_WORKERS;
    MPI_Irecv(&tokenBuf, 2, MPI_INT, source, MPI_TERMINATE_TAG, MPI_COMM_WORLD, &pendingToken);

    int processed = 0;
    bool allNegationDone = false;
    while (!allNegationDone) {
        if (NEG_VOTE) OUTPUT("Next round");

        while(!W->empty() || !terminationDetection()) {
            Edge *e = nullptr;

            STATUS();
            if (try_recv_messages() > 0) {
                //if some messages have been received
                termination_flag = FLAG_DIRTY;
            }

            if (!W->empty()) {
                termination_flag = FLAG_DIRTY;
                STATUS();
                e = W->pop();
                processed += 1;
                if(SEARCH) OUTPUT(e->to_string());
                if(e->source->IsNegated){
                    processNegationEdge(e);
                    STATUS();
                }
                else {
                    processHyperEdge(e);
                    STATUS();
                }
            }

            if(v->assignment == ONE || v->assignment == CZERO) {break;}
        }

        if(v->assignment == ONE || v->assignment == CZERO) {break;}

        int candidate = v->query->max_depth;
        if (NEG_VOTE) OUTPUT("Negation round proposal: ", candidate);
        if (!N.empty()) {
            candidate = N.top()->source->query->max_depth;
        }

        int winner = 0;
        MPI_Allreduce(&candidate, &winner, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
        if (NEG_VOTE) OUTPUT("Negation round winner: ", winner);

        //We have to vote about emptiness before actually processing the edges.
        //That way we ensure that we terminate only if we are all empty and we couldn't have
        //inserted anything into W or sent any messages.
        bool empty = N.empty();
        if (NEG_VOTE) OUTPUT("All empty proposal: ", empty);
        MPI_Allreduce(&empty, &allNegationDone, 1, MPI_C_BOOL, MPI_LAND, MPI_COMM_WORLD);

        if (NEG_VOTE) OUTPUT("All empty result: ", allNegationDone);


        while (!N.empty() && N.top()->source->query->max_depth <= winner) {            
            Edge *e = N.top();
            N.pop();
            processed += 1;
            processNegationEdge(e);
        }        
    }

    //MPI_Finalize();
    std::cout << "Counter: " << counter << std::endl;
    std::cout << "Processed " << processed << std::endl;
    std::cout << "Sent: " << wrapper.exported_counter << std::endl;
    std::cout << "Recv: " << wrapper.received_counter << std::endl;
    std::cout << "Size: " << wrapper.sent_size << std::endl;
    return (v->assignment == ONE) ? true : false;
}

template<class ...args>
void DistCZeroFPA::output(args... arguments)
{
#if LOGGING || PRINTING
    std::stringstream ss;
    ss << to_string(arguments...) << std::endl;
    std::string str = ss.str();
#else
#endif

#if LOGGING
    assert(ostream.is_open());
    ostream << str;
#else
#endif

#if PRINTING
    std::cout << str;
#else
#endif
}

std::string DistCZeroFPA::to_string()
{
    return std::string();
}

template<class T, class ...Ts>
std::string DistCZeroFPA::to_string(T head, Ts... tail)
{
    std::stringstream ss;
    ss << head << to_string(tail...);
    return ss.str();
}

}
*/


#include "DistCZeroFPA.h"
#include "assert.h"

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
                Message m(comm->rank(), type, nextMessageId(), c);
                comm->sendMessage(i, m);
            }
        }
    }

    for (std::pair<int, long> interest : c->interested) {
        if (interest.second > 0) {
            Message m(comm->rank(), type, nextMessageId(), c);
            comm->sendMessage(interest.first, m);
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
    if (c->assignment == UNKNOWN && c->hasActiveDependencies()) {
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
            Message m(comm->rank(), Message::REQUEST, nextMessageId(), c);
            comm->sendMessage(partition->ownerId(c), m);
        }
    }
}

void Algorithm::DistCZeroFPA::halt(Configuration *c)
{
    if (!(c->hasActiveDependencies() || c == v || c->isDone())) {
        c->assignment = UNKNOWN;
        if (partition->ownerId(c) == comm->rank()) {
            for (Edge *s : c->successors) {
                strategy->pushEdge(s);
            }
        } else {
            Message m(comm->rank(),Message::HALT, nextMessageId(), c);
            comm->sendMessage(partition->ownerId(c), m);
        }
    }
}


void Algorithm::DistCZeroFPA::processMessage(Message *m)
{
    Configuration *c = m->configuration;

    if (m->type == Message::ANSWER_ONE) {
        finalAssign(c, ONE);
    } else if (m->type == Message::ANSWER_ZERO) {
        finalAssign(c, CZERO);
    } else if (m->type == Message::REQUEST) {
        if (c->isDone()) {
            Message::Type t = c->assignment == ONE ? Message::ANSWER_ONE : Message::ANSWER_ZERO;
            Message out(comm->rank(), t, nextMessageId(), c);
            comm->sendMessage(m->sender, out);
        } else {
            c->updateInterest(m->sender, m->id);
            if (c->hasActiveDependencies() && c->assignment == UNKNOWN) {
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
                halt(e->requested);
                e->requested = nullptr;
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else {
            assert(lastUndecided != nullptr);
            e->requested = lastUndecided;
            lastUndecided->dependency_set.push_back(e);
            explore(lastUndecided);
        }
    } else if (e->requested != nullptr) {   //halt!
        halt(e->requested);
        e->requested = nullptr;
    }
}

void Algorithm::DistCZeroFPA::processNegationEdge(Edge *e)
{
    if(e->source->assignment == ZERO && !e->is_deleted){
        assert(e->targets.size() == 1);
        Configuration *target = e->targets.front();

        if (target->assignment == ONE) {
            e->source->removeSuccessor(e);
            if (e->requested != nullptr) {
                //technically this shouldn't happen - maybe get rid of it?
                halt(e->requested);
                e->requested = nullptr;
            }
            if (e->source->successors.empty()) {
                finalAssign(e->source, CZERO);
            }
        } else if (target->assignment == CZERO || target->assignment == ZERO) {
            finalAssign(e->source, ONE);
        } else {
            assert(target->assignment == UNKNOWN);
            e->requested = target;
            target->dependency_set.push_back(e);
            explore(target);
        }        
    } else if (e->requested != nullptr) {
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


Algorithm::DistCZeroFPA::DistCZeroFPA(Algorithm::PartitionFunction *partition, Communicator *comm) :
    FixedPointAlgorithm(), partition(partition), comm(comm)
{ }

bool Algorithm::DistCZeroFPA::search(BasicDependencyGraph &t_graph, AbstractSearchStrategy &t_strategy)
{
    this->graph = &t_graph;
    this->strategy = &t_strategy;
    this->v = graph->initialConfiguration();

    termination_flag = FLAG_DIRTY;
    if (partition->ownerId(v)) {
        explore(v);
    }

    int canPick = 0;

    while (canPick <= v->getDistance()) {
        while (!(strategy->empty() && terminationDetection())) {

            std::pair<int, Message> message = comm->recvMessage();
            while (message.first >= 0) {
                strategy->pushMessage(message.second);
                message = comm->recvMessage();
            }

            Edge *e;
            Message *m;
            AbstractSearchStrategy::TaskType type = strategy->pickTask(e, m);
            if (type == AbstractSearchStrategy::EDGE) {
                termination_flag = FLAG_DIRTY;
                if (e->is_negated) {
                    processNegationEdge(e);
                } else {
                    processHyperEdge(e);
                }
            } else if (type == AbstractSearchStrategy::MESSAGE) {
                termination_flag = FLAG_DIRTY;
                processMessage(m);
            } else {
                //this is ok if we are waiting for termination detection
            }

            if (v->isDone()) break;
        }

        if (v->isDone()) break;

        int candidate = v->getDistance() + 1;
        if (!unsafe_N.empty()) {
            candidate = unsafe_N.top()->source->getDistance();
        }

        //if we are not master, send proposal
        if (comm->rank() != 0) {
            comm->sendDistance(0, candidate);
        } else {
            //if we are master, collect proposals and compute minimum
            for (int i=1; i<comm->size(); i++) {
                std::pair<int, int> proposal = comm->recvDistance();
                while (proposal.first < 0) {
                    proposal = comm->recvDistance();
                }
                if (proposal.second < candidate) {
                    candidate = proposal.second;
                }
            }
        }

        //broadcast results of vote to everyone
        int winner = candidate;
        comm->broadcastDistance(0, winner);

        //copy safe edges into strategy
        while (!unsafe_N.empty() && unsafe_N.top()->source->getDistance() <= winner) {
            strategy->pushEdge(unsafe_N.top());
            unsafe_N.pop();
        }
    }

    return (v->assignment == ONE) ? true : false;
}

