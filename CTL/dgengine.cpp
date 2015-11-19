#include "dgengine.h"
#include "../CTLParser/CTLParser.h"

#include <string.h>

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool t_CZero){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
    _CZero = t_CZero;
}

std::list<Edge*> DGEngine::successors(Configuration& v) {
    std::list<Edge*> succ;
    //All
    if(v.query->quantifier == A){
        //All Until
        if(v.query->path == U){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);
            succ.push_back(e);

            auto targets = nextState (*(v.marking));

            if(!targets.empty()){
                Edge* e1 = new Edge(&v);
                Configuration* b = createConfiguration(*(v.marking), *(v.query->first));
                e1->targets.push_back(b);

                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e1->targets.push_back(c);
                }
            }

            #ifdef PP
            edgePrinter(e);edgePrinter(e1);
            #endif

        } //All Until end

        //All Next begin
        else if(v.query->path == X){
            auto targets = nextState(*v.marking);
            if(!targets.empty())
            {
                Edge* e = new Edge(&v);

                for (auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query->first));
                    e->targets.push_back(c);
                }
            }
        } //All Next End

        //All Finally start
        else if(v.query->path == F){
            Edge* e = new Edge(&v);
            Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
            e->targets.push_back(c);
            succ.push_back(e);

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Edge* e1 = new Edge();
                e1->source = &v;

                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e1->targets.push_back(c);
                }
            }
        }//All Finally end
    } //All end

    //Exists start
    else if (v.query->quantifier == E){

        //Exists Untill start
        if(v.query->path == U){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);
            succ.push_back(e);

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c1 = createConfiguration(*m, *(v.query));
                    e->targets.push_back(c);
                    e->targets.push_back(c1);
                    succ.push_back(e);
                }
            }
        } //Exists Until end

        //Exists Next start
        else if(v.query->path == X){
            auto targets = nextState(*(v.marking));
            CTLTree* query = v.query->first;

            if(!targets.empty())
            {
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c = createConfiguration(*m, *query);
                    e->targets.push_back(c);
                    succ.push_back(e);
                }
            }
        }//Exists Next end

        //Exists Finally start
        else if(v.query->path == F){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);
            succ.push_back(e);

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e->targets.push_back(c);
                    succ.push_back(e);
                }
            }
        }//Exists Finally end
    } //Exists end

    //And start
    else if (v.query->quantifier == AND){
        Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
        Configuration* c1 = createConfiguration(*(v.marking), *(v.query->second));
        Edge* e = new Edge(&v);
        e->targets.push_back(c);
        e->targets.push_back(c1);
        succ.push_back(e);
    } //And end

    //Or start
    else if (v.query->quantifier == OR){
        Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
        Configuration* c1 = createConfiguration(*(v.marking), *(v.query->second));
        Edge* e = new Edge(&v);
        Edge* e1 = new Edge(&v);
        e->targets.push_back(c);
        e1->targets.push_back(c1);
        succ.push_back(e);
        succ.push_back(e1);
    } //Or end

    //Negate start
    else if (v.query->quantifier == NEG){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);
            succ.push_back(e);
    } //Negate end

    //Evaluate Query Begin
    else {
        Edge* e = new Edge(&v);
        e->targets.push_back(&v);
        if (evaluateQuery(v)){
            assignConfiguration(v, ONE);
        } else {
            assignConfiguration(v, CZERO);
        }
        succ.push_back(e);
    }
    return succ;
}


bool DGEngine::evaluateQuery(Configuration &t_config){

    CTLTree* query = t_config.query;

    if (t_config.query->a.isFireable) {
        std::list<int> transistions = calculateFireableTransistions(*(t_config.marking));

        for (auto t : transistions) {
            int j = 0;
            while(query->a.fireset[j] != NULL){
                if (strcmp(_net->transitionNames()[t].c_str(), query->a.fireset[j]) == 0){
                    return true;
                }
                j++;
            }
        }
        return false;
    }

    int less = query->a.tokenCount.intSmaller;
    int greater= query->a.tokenCount.intLarger;

    if( less == -1 ){
        int index = indexOfPlace(query->a.tokenCount.placeSmaller);
        less = t_config.marking->Value()[index];
    }

    if (greater == -1){
        int index = indexOfPlace(query->a.tokenCount.placeLarger);
        greater = t_config.marking->Value()[index];
    }

    return less < greater;
}

int DGEngine::indexOfPlace(char *t_place){
    for (int i = 0; i < _nplaces; i++) {
        if (0 == (strcmp(t_place, _net->placeNames()[i].c_str()))){
                //cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                return i;
        }
    }
    return -1;
}

void DGEngine::assignConfiguration(Configuration& t_config, Assignment t_assignment){

    if(t_config.IsNegated){
        //Under zero means Assignment enum is either ZERO og CZERO
        if(t_assignment < 0){
            t_config.assignment = ONE;
        }
        else if(_CZero){
            t_config.assignment = CZERO;
        }
        else { t_config.assignment = ZERO; }
    }
    else {
        if(t_assignment > 0){
            t_config.assignment = t_assignment;
        }
        else if(_CZero){
            t_config.assignment = CZERO;
        }
        else { t_config.assignment = ZERO; }
    }
}

std::list<Marking*> DGEngine::nextState(Marking& t_marking){

    std::list<int> fireableTransistions = calculateFireableTransistions(t_marking);
    std::list<Marking*> nextStates;

    auto iter = fireableTransistions.begin();
    auto iterEnd = fireableTransistions.end();

    //Create new markings for each fireable transistion
    for(; iter != iterEnd; iter++){
        nextStates.push_back(createMarking(t_marking, *iter));
    }

    //return the set of reachable markings
    return nextStates;
}

std::list<int> DGEngine::calculateFireableTransistions(Marking &t_marking){

    std::list<int> fireableTransistions;

    for(int t = 0; t < _ntransitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < _nplaces; p++){
            if(t_marking[p] < _net->inArc(p,t))
                transitionFound = false;
        }

        if(transitionFound)
            fireableTransistions.push_back(t);
    }
    return fireableTransistions;
}

Configuration* DGEngine::createConfiguration(Marking &t_marking, CTLTree& t_query){
    Configuration* newConfig = new Configuration();

    newConfig->marking = &t_marking;
    newConfig->query = &t_query;

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->IsNegated = true;
    }

    return *(Configurations.insert(newConfig).first);
}

Marking* DGEngine::createMarking(const Marking& t_marking, int t_transition){
        Marking* new_marking = new Marking();

        new_marking->CopyMarking(t_marking);

        for(int p = 0; p < _nplaces; p++){
            int place = (*new_marking)[p] - _net->inArc(p,t_transition);
            (*new_marking)[p] = place + _net->outArc(t_transition,p);
        }

        //insert function either inserts or finds the element
        return *(Markings.insert(new_marking).first);
    }

    void DGEngine::RunEgineTest(PetriEngine::PetriNet net, PetriEngine::MarkVal m0){
        /*Marking *initmarking = new Marking(m0);
        Marking *testmarking = createMarking(*initmarking, 0);
        CTLParser testparser = new CTLParser();
        CTLFormula testquery[1];
        //testparser.ParseXMLQuery(buffer, testquery);
        
        Configuration *testconfig = createConfiguration(m0, testquery[0].Query);*/
    }
    
}