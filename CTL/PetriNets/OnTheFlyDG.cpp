#include "OnTheFlyDG.h"

#include <string.h>

using namespace DependencyGraph;

namespace PetriNets {

OnTheFlyDG::OnTheFlyDG(
        PetriEngine::PetriNet *t_net,
        PetriEngine::MarkVal *t_initial,
        PNMLParser::InhibitorArcList t_inhibitorArcs
):
    net(t_net), inhibitorArcs(t_inhibitorArcs), n_transitions(t_net->numberOfTransitions()),
    n_places(t_net->numberOfPlaces())
{    
    initial_marking = new Marking(t_initial, n_places);
    markings.insert(initial_marking);
    cached_successors.resize(t_net->numberOfTransitions());
}


OnTheFlyDG::~OnTheFlyDG()
{
    cleanUp();
    delete initial_marking;
}

void OnTheFlyDG::successors(DependencyGraph::Configuration *c)
{
    PetriConfig *v = static_cast<PetriConfig*>(c);
    std::vector<Edge*> succ;
    //All
    if(v->query->quantifier == A){
        //All Until
        if(v->query->path == U){
            //first deal with the right side
            Edge *right = NULL;
            if (!v->query->second->isTemporal) {
                //right side is not temporal, eval it right now!
                bool valid = fastEval(*(v->query->second), *(v->marking));
                if (valid) {    //satisfied, no need to go through successors
                    succ.push_back(new Edge(*v));
                    v->successors = succ;
                    return;
                } //else: It's not valid, no need to add any edge, just add successors
            } else {
                //right side is temporal, we need to evaluate it as normal
                Configuration* c = createConfiguration(*(v->marking), *(v->query->second));
                right = new Edge(*v);
                right->targets.push_back(c);
            }

            //if we got here, either right side is temporal or it is not satisfied
            bool valid = false;
            Configuration *left = NULL;
            if (!v->query->first->isTemporal) {
                //left side is not temporal, eval it right now!
                valid = fastEval(*(v->query->first), *(v->marking));
            } else {
                //left side is temporal, include it in the edge
                left = createConfiguration(*(v->marking), *(v->query->first));
            }

            if (valid || left != NULL) {    //if left side is guaranteed to be not satisfied, skip successor generation
                auto targets = nextState (*(v->marking));

                if(!targets.empty()){
                    Edge* leftEdge = new Edge(*v);

                    for(auto m : targets){
                        Configuration* c = createConfiguration(*m, *(v->query));
                        leftEdge->targets.push_back(c);
                    }
                    if (left != NULL) {
                        leftEdge->targets.push_back(left);
                    }
                    succ.push_back(leftEdge);
                }
            } //else: Left side is not temporal and it's false, no way to succeed there...

            if (right != NULL) {
                succ.push_back(right);
            }


        } //All Until end

        //All Next begin
        else if(v->query->path == X){
            auto targets = nextState(*v->marking);
            if (v->query->first->isTemporal) {   //regular check
                Edge* e = new Edge(*v);
                for (auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v->query->first));
                    e->targets.push_back(c);
                }
                succ.push_back(e);
            } else {
                bool allValid = true;
                for (auto m : targets) {
                    bool valid = fastEval(*(v->query->first), *m);
                    if (!valid) {
                        allValid = false;
                        break;
                    }
                }
                if (allValid) {
                    succ.push_back(new Edge(*v));
                    v->successors = succ;
                    return;
                }
            }
        } //All Next End

        //All Finally start
        else if(v->query->path == F){
           // std::cout << "do AF" << std::endl;
            Edge *subquery = NULL;
            if (!v->query->first->isTemporal) {
                bool valid = fastEval(*(v->query->first), *(v->marking));
                if (valid) {
                    succ.push_back(new Edge(*v));
                    v->successors = succ;
                    return;
                }
            } else {
                subquery = new Edge(*v);
                Configuration* c = createConfiguration(*(v->marking), *(v->query->first));
                subquery->targets.push_back(c);
            }

           // std::cout << "Succ for: " << std::endl;
//            v->printConfiguration();
            auto targets = nextState(*(v->marking));

            if(!targets.empty()){
                Edge* e1 = new Edge(*v);

                //std::cout << "Targets: " << std::endl;
                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v->query));
                    //c->printConfiguration();
                    e1->targets.push_back(c);
                }
                succ.push_back(e1);
            } else {
                //std::cout << "Empty" << std::endl;
            }

            if (subquery != NULL) {
                succ.push_back(subquery);
            }

        }//All Finally end
    } //All end

    //Exists start
    else if (v->query->quantifier == E){

        //Exists Untill start
        if(v->query->path == U){
            Edge *right = NULL;
            if (v->query->second->isTemporal) {
                Configuration* c = createConfiguration(*(v->marking), *(v->query->second));
                right = new Edge(*v);
                right->targets.push_back(c);
            } else {
                bool valid = fastEval(*(v->query->second), *(v->marking));
                if (valid) {
                    succ.push_back(new Edge(*v));
                    v->successors = succ;
                    return;
                }   // else: right condition is not satisfied, no need to add an edge
            }

            auto targets = nextState(*(v->marking));

            if(!targets.empty()){
                Configuration *left = NULL;
                bool valid = false;
                if (v->query->first->isTemporal) {
                    left = createConfiguration(*(v->marking), *(v->query->first));
                } else {
                    valid = fastEval(*(v->query->first), *(v->marking));
                }
                if (left != NULL || valid) {
                    for(auto m : targets) {
                        Edge* e = new Edge(*v);
                        Configuration* c1 = createConfiguration(*m, *(v->query));
                        e->targets.push_back(c1);
                        if (left != NULL) {
                            e->targets.push_back(left);
                        }
                        succ.push_back(e);
                    }
                }
            }

            if (right != NULL) {
                succ.push_back(right);
            }

        } //Exists Until end

        //Exists Next start
        else if(v->query->path == X){
            auto targets = nextState(*(v->marking));
            CTLTree* query = v->query->first;

            if(!targets.empty())
            {
                if (query->isTemporal) {    //have to check, no way to skip that
                    for(auto m : targets){
                        Edge* e = new Edge(*v);
                        Configuration* c = createConfiguration(*m, *query);
                        e->targets.push_back(c);
                        succ.push_back(e);
                    }
                } else {
                    for(auto m : targets) {
                        bool valid = fastEval(*query, *m);
                        if (valid) {
                            succ.push_back(new Edge(*v));
                            v->successors = succ;
                            return;
                        }   //else: It can't hold there, no need to create an edge
                    }
                }
            }
        }//Exists Next end

        //Exists Finally start
        else if(v->query->path == F){
            Edge *subquery = NULL;
            if (!v->query->first->isTemporal) {
                bool valid = fastEval(*(v->query->first), *(v->marking));
                if (valid) {
                    succ.push_back(new Edge(*v));
                    v->successors = succ;
                    return;
                }
            } else {                
                Configuration* c = createConfiguration(*(v->marking), *(v->query->first));
                subquery = new Edge(*v);
                subquery->targets.push_back(c);                
            }

            auto targets = nextState(*(v->marking));

            if(!targets.empty()){
                for(auto m : targets){
                    Edge* e = new Edge(*v);
                    Configuration* c = createConfiguration(*m, *(v->query));
                    e->targets.push_back(c);
                    succ.push_back(e);
                }
            }

            if (subquery != NULL) {
                succ.push_back(subquery);
            }

        }//Exists Finally end
    } //Exists end

    //And start
    else if (v->query->quantifier == AND){

        //Check if left is false
        if(!v->query->first->isTemporal){
            if(!fastEval(*(v->query->first), *v->marking))
                //query cannot be satisfied, return empty succ set
                return;
        }

        //check if right is false
        if(!v->query->second->isTemporal){
            if(!fastEval(*(v->query->second), *v->marking))
                return;
        }

        Edge *e = new Edge(*v);

        //If we get here, then either both propositions are true(should not be possible)
        //Or a temporal operator and a true proposition
        //Or both are temporal
        if(v->query->first->isTemporal){
            e->targets.push_back(createConfiguration(*v->marking, *(v->query->first)));
        }
        if(v->query->second->isTemporal){
            e->targets.push_back(createConfiguration(*v->marking, *(v->query->second)));
        }
        succ.push_back(e);
    } //And end

    //Or start
    else if (v->query->quantifier == OR){

        //Check if left is true
        if(!v->query->first->isTemporal){
            if(fastEval(*(v->query->first), *v->marking)){
                //query is satisfied, return
                succ.push_back(new Edge(*v));
                v->successors = succ;
                return;
            }
        }

        if(!v->query->second->isTemporal){
            if(fastEval(*(v->query->second), *v->marking)){
                succ.push_back(new Edge(*v));
                v->successors = succ;
                return;
            }
        }

        //If we get here, either both propositions are false
        //Or one is false and one is temporal
        //Or both temporal
        if(v->query->first->isTemporal){
            Edge *e = new Edge(*v);
            e->targets.push_back(createConfiguration(*v->marking, *(v->query->first)));
            succ.push_back(e);
        }
        if(v->query->second->isTemporal){
            Edge *e = new Edge(*v);
            e->targets.push_back(createConfiguration(*v->marking, *(v->query->second)));
            succ.push_back(e);
        }
    } //Or end

    //Negate start
    else if (v->query->quantifier == NEG){
            Configuration* c = createConfiguration(*(v->marking), *(v->query->first));
            Edge* e = new Edge(*v);
            e->targets.push_back(c);
            succ.push_back(e);
    } //Negate end

    //Evaluate Query Begin
    else {
        //We should never get here anymore.
        //v->configPrinter();
        //assert(false);
        if (evaluateQuery(*v->query, *v->marking)){
            succ.push_back(new Edge(*v));
        }
    }

    v->successors = succ;
}

bool OnTheFlyDG::evaluateQuery(CTLTree &query, Marking &marking)
{
    bool result = false;

    if (query.a.isFireable) {
        std::list<int> transistions = calculateFireableTransistions(marking);

        for (auto t : transistions) {
            int fs_transition = 0;
            for(fs_transition = 0; fs_transition < query.a.firesize; fs_transition++){
                int dpc_place = 0;
                int truedependencyplaces = 0;
                for (dpc_place = 0; dpc_place < query.a.fireset[fs_transition].sizeofdenpencyplaces; dpc_place++){

                    if((query.a.fireset[fs_transition].denpencyplaces[dpc_place].intSmaller - 1) < marking[query.a.fireset[fs_transition].denpencyplaces[dpc_place].placeLarger]){
                        truedependencyplaces++;
                    }
                }
                if (truedependencyplaces == query.a.fireset[fs_transition].sizeofdenpencyplaces){
                    result = true;
                }
            }
        }
        return result;
    }

    int less = query.a.cardinality.intSmaller;
    int greater= query.a.cardinality.intLarger;
    if( less == -1 ){
        int i = 0;
        less = 0;
        for (i = 0; i < query.a.cardinality.placeSmaller.sizeoftokencount; i++){
            int index = query.a.cardinality.placeSmaller.cardinality[i];
            less += marking[index];
        }
    }
    if (greater == -1){
        int i = 0;
        greater = 0;
        for (i = 0; i < query.a.cardinality.placeLarger.sizeoftokencount; i++){
            int index = query.a.cardinality.placeLarger.cardinality[i];
            greater += marking[index];
        }
    }

    result = less <= greater;
    return result;
}

bool OnTheFlyDG::fastEval(CTLTree &query, Marking &marking)
{
    assert(!query.isTemporal);
    if (query.quantifier == AND){
        return fastEval(*query.first, marking) && fastEval(*query.second, marking);
    } else if (query.quantifier == OR){
        return fastEval(*query.first, marking) || fastEval(*query.second, marking);
    } else if (query.quantifier == NEG){
        return !fastEval(*query.first, marking);
    } else {
        return evaluateQuery(query, marking);
    }
}

Configuration* OnTheFlyDG::initialConfiguration()
{
    //initial marking is inserted into the set in the constructor or after cleanup
    return createConfiguration(*initial_marking, *query);
}

int OnTheFlyDG::indexOfPlace(char *t_place){
    for (int i = 0; i < n_places; i++) {
        if (0 == (strcmp(t_place, net->placeNames()[i].c_str()))){
                //cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                return i;
        }
    }
    return -1;
}

std::vector<Marking*> OnTheFlyDG::nextState(Marking& t_marking){

    if(cached_marking == &t_marking){
        return cached_successors;
    }

    auto fireable = calculateFireableTransistions(t_marking);
    std::vector<Marking*> nextStates;

//    if(fireable.empty())
//        return nextStates;

    //Update cache
    size_t old_size = cached_successors.capacity();
    cached_marking = &t_marking;
    cached_successors.clear();

    assert(old_size == cached_successors.capacity());

    for(int t : fireable){
        Marking *m2 = createMarking(t_marking, t);
        nextStates.push_back(m2);
        cached_successors.push_back(m2);
    }

    return nextStates;
}

std::list<int> OnTheFlyDG::calculateFireableTransistions(Marking &t_marking){

    std::list<int> fireableTransistions;
    
    for(int t = 0; t < n_transitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < n_places; p++){
            if(t_marking[p] < net->inArc(p,t)){
                transitionFound = false;
                break;
            }
        }

        if(transitionFound){ //Inhibitor check
            for(auto inhibitor : inhibitorArcs){
                if(inhibitor.weight > 0 && net->transitionNames()[t].compare(inhibitor.target) == 0 ){
                    for(int p = 0; p < n_places; p++){
                        if(net->placeNames()[p].compare(inhibitor.source) == 0 && t_marking[p] > inhibitor.weight){
                            transitionFound = false;
                            break;
                        }
                    }   
                }
                if(!transitionFound)
                    break;
            }
        }

        if(transitionFound)
            fireableTransistions.push_back(t);
    }
    
    return fireableTransistions;
}


void OnTheFlyDG::cleanUp()
{
    for (Marking *m : markings) {
        for (PetriConfig *c : m->successors) {
            delete c;
        }
        m->successors.resize(0);
        delete m;
    }
    markings.clear();
    markings.insert(initial_marking);
}

std::pair<int, int *> OnTheFlyDG::serialize(SearchStrategy::Message &m)
{
    //message structure: sender | type | id | query_id | configuration;
    PetriConfig *v = static_cast<PetriConfig*>(m.configuration);
    int size = v->marking->length() + 4;
    int *buffer = (int*) malloc(sizeof(int) * size);
    buffer[0] = m.sender;
    buffer[1] = m.type;
    buffer[2] = m.id;
    buffer[3] = v->query->id;
    for (int i=0; i<v->marking->length(); i++) {
        buffer[i+4] = v->marking->value()[i];
    }
    return std::pair<int, int*>(size, buffer);
}

SearchStrategy::Message OnTheFlyDG::deserialize(int *message, int messageSize)
{
    int *markingStart = message + 4;
    Marking *marking = new Marking((PetriEngine::MarkVal*) markingStart, messageSize - 4);

    auto result = markings.find(marking);

    if(result == markings.end()){
        marking = *(markings.insert(marking).first);
    }
    else{
        delete marking;
    }

    CTLTree *query = findQueryById(message[3], this->query);
    assert(query != nullptr);

    Configuration *c = createConfiguration(*marking, *query);

    SearchStrategy::Message m(message[0], (SearchStrategy::Message::Type) message[1], message[2], c);
    return m;
}

CTLTree *OnTheFlyDG::findQueryById(int id, CTLTree *root)
{
    CTLTree * result = nullptr;
    if (root->id == id) {
        result = root;
    } else if (root->quantifier != EMPTY) {
        if (root->first != nullptr) {
            result = findQueryById(id, root->first);
        }
        if (root->second != nullptr && result == nullptr) {
            result = findQueryById(id, root->second);
        }
    }
    return result;
}

void OnTheFlyDG::setQuery(CTLTree *query)
{
    this->query = query;
}

Configuration *OnTheFlyDG::createConfiguration(Marking &t_marking, CTLTree &t_query)
{    /*
    std::cout << "Already has configs:" << std::endl;
    t_marking.print();
    CTLParser p;
    p.printQuery(&t_query);*/
    for(PetriConfig* c : t_marking.successors){
        //c->printConfiguration();
        if(c->query == &t_query)
            //std::cout << "Return this one!" << std::endl;
            return c;
    }

    PetriConfig* newConfig = new PetriConfig(&t_marking, &t_query);

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->is_negated = true;
    }

    t_marking.successors.push_back(newConfig);
    return newConfig;
}



Marking *OnTheFlyDG::createMarking(const Marking& t_marking, int t_transition){
    Marking* new_marking = new Marking();

    new_marking->copyMarking(t_marking);

    for(int p = 0; p < n_places; p++){
        int place = (*new_marking)[p] - net->inArc(p,t_transition);
        (*new_marking)[p] = place + net->outArc(t_transition,p);
    }

    auto result = markings.find(new_marking);

    if(result == markings.end()){
        return *(markings.insert(new_marking).first);
    }
    else{
        delete new_marking;
    }

    return *result;
}

}//PetriNet
