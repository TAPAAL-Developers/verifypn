#include "dgengine.h"
#include "../CTLParser/CTLParser.h"

#include <string.h>
#include <iostream>
#include <stack>

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool t_CZero){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
    _CZero = t_CZero;
}

void DGEngine::search(CTLTree *t_query){
    //pNetPrinter(_net, _m0);
    #ifdef DEBUG
    cout << "--------------------- NEW QUERY-------------------------------------------\n" << flush;
    #endif

    Marking* firstMarking = new Marking(_m0, _nplaces);
    Configuration* v0 = createConfiguration(*firstMarking, *t_query);

    #ifdef DEBUG
    cout << ":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n::::::::::::::::: Initial Configuration\n::::::::::::::::";
    #endif
    #ifdef PP
    configPrinter(v0);
    #endif
    #ifdef DEBUG
    cout << ":\n:::::::::::::::::\n:::::::::::::::::\n:::::::::::::::::\n" << flush;
    #endif
    #ifdef DEBUG
    CTLParser ctlParser = CTLParser();
    ctlParser.printQuery(query);
    #endif

    Markings.clear();
    _querySatisfied = localSmolka(*v0);
}

bool DGEngine::localSmolka(Configuration &v){

    v.assignment = ZERO;
    std::stack<Edge*> W;
    auto initialSucc = successors(v);

     std::cout << "Starting while loop - size of W:" << W.size() << std::endl;
    std::cout << "--------- NUMBER OF EDGES IN w NOW AND THEIR LOOK "<< W.size() << "\n" << std::flush;
        for(auto c : initialSucc){
        c->edgePrinter();
        }
        //#endif


    //cout << "FIRST config :\n" << flush;
   // configPrinter(v);

    for(auto s : initialSucc)
        W.push(s);

    while (!W.empty()) {
        //#ifdef DEBUG
       
        int i = 0;  
        Edge* e = W.top();
        W.pop();

        /*****************************************************************/
        /*Data handling*/
        #ifdef DEBUG
        cout<<"Starting Data Handling\n"<<flush;
        #endif
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        int targetUKNOWNassignments = 0;
        bool czero = false;

        for(auto c : e->targets){
            #ifdef DEBUG
            cout<<"Target "<< i << " out of " << e.targets.size() << " assignment: "<< *(e.targets[i].assignment) << "\n"<<flush;
            #endif
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
                targetUKNOWNassignments++;
            }
        }

        #ifdef DEBUG
        cout<<"Completed Data Handling\nResult:\n"<<flush;
        cout<<"ONE's: "<< targetONEassignments<<"\n" <<flush;
        cout<<"CZERO: "<< czero << endl << flush;
        cout<<"Zero's: "<< targetZEROassignments<<"\n" <<flush;
        cout<<"Unknowns's: "<< targetUNKNOWNassignments<<"\n" <<flush;
        #endif

        /******************************************************************/
        //Case: One
        if (targetONEassignments == e->targets.size()){
            #ifdef DEBUG
            cout<<"All assignments were ONE"<<endl;
            #endif
            assignConfiguration(*(e->source), ONE);

            if(*(e->source) == v)
                return v.assignment == ONE ? true : false;

            for(auto edge : e->source->DependencySet)
                W.push(edge);
        }
        /*****************************************************************/
        // Case: CZERO
        else if(czero){

            #ifdef DEBUG
            cout<<"Certain Zero in target configurations"<<endl;
            #endif

            if(e->source->Successors.size() == 1){
                assignConfiguration(*(e->source), CZERO);

                if(*(e->source) == v)
                    return v.assignment == ONE ? true : false;
            }
            e->source->removeSuccessor(e);
        }
        /*****************************************************************/
        // Case: Negated
        else if(e->source->IsNegated){

            #ifdef DEBUG
            cout<<"Negated: Calling smolka recursively"<<endl;
            #endif

            Configuration* negConfig = *(e->targets.begin());
            localSmolka(*negConfig);

            if(negConfig->assignment == ONE){
                assignConfiguration(*(e->source), CZERO);
                e->source->removeSuccessor(e);

                if(_CZero){
                    for(auto edge : e->source->DependencySet)
                        W.push(edge);
                }
            }
            else {
                assignConfiguration(*(e->source), ONE);

                for(auto edge : e->source->DependencySet)
                    W.push(edge);
            }
        }
        /*****************************************************************/
        // CASE: ZERO
        else if ( targetZEROassignments > 0){
            for(auto c : e->targets){
                if(c->assignment == ZERO) c->DependencySet.push_back(e);
            }
        }
        /*****************************************************************/
        // Case: UNKNOWN
        else if (targetUKNOWNassignments > 0){
            #ifdef DEBUG
            cout<<"All assignments were unknown"<<endl;
            #endif
            for(auto c : e->targets){
                if(c->assignment == UNKNOWN){
                    c->assignment = ZERO;
                    c->DependencySet.push_back(e);
                    for(auto s : successors(*c)){
                        s->edgePrinter();
                        W.push(s);
                    }
                }
            }
        }

    }
    #ifdef DEBUG
    cout<<"The final assignment of the initial configuration is: " << *(v.assignment)<<endl;
    #endif
    //cout << "the final value is: " << *(v.assignment) << "\n" << flush;
    //assignConfiguration(v, *(v.assignment));
    return (v.assignment == ONE) ? true : false;
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
        std::cout << "printing eval edge" << std::flush;
        e->edgePrinter();
    }

    v.Successors = succ;
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