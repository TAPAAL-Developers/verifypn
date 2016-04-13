#include "OnTheFlyDG.h"

#include <string.h>

namespace ctl{

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net, PetriEngine::MarkVal *t_initial, PNMLParser::InhibitorArcList inhibitorArcs):
    DependencyGraph(t_net, t_initial, inhibitorArcs)
{
    _compressoption = false;
}


bool OnTheFlyDG::fastEval(CTLTree &query, Marking &marking) {
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

std::list<Edge *> OnTheFlyDG::successors(Configuration &v)
{
    std::list<Edge*> succ;
    //All
    if(v.query->quantifier == A){
        //All Until
        if(v.query->path == U){
            //first deal with the right side
            Edge *right = NULL;
            if (!v.query->second->isTemporal) {
                //right side is not temporal, eval it right now!
                bool valid = fastEval(*(v.query->second), *(v.marking));
                if (valid) {    //satisfied, no need to go through successors
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                } //else: It's not valid, no need to add any edge, just add successors
            } else {
                //right side is temporal, we need to evaluate it as normal
                Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
                right = new Edge(&v);
                right->targets.push_back(c);
            }
            
            //if we got here, either right side is temporal or it is not satisfied   
            bool valid = false;
            Configuration *left = NULL;
            if (!v.query->first->isTemporal) {
                //left side is not temporal, eval it right now!
                valid = fastEval(*(v.query->first), *(v.marking));
            } else {
                //left side is temporal, include it in the edge
                left = createConfiguration(*(v.marking), *(v.query->first));
            }         

            if (valid || left != NULL) {    //if left side is guaranteed to be not satisfied, skip successor generation
                auto targets = nextState (*(v.marking));

                if(!targets.empty()){   
                    Edge* leftEdge = new Edge(&v);

                    for(auto m : targets){
                        Configuration* c = createConfiguration(*m, *(v.query));
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
        else if(v.query->path == X){
            auto targets = nextState(*v.marking);
            if (v.query->first->isTemporal) {   //regular check
                Edge* e = new Edge(&v);
                for (auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query->first));
                    e->targets.push_back(c);
                }
                succ.push_back(e);
            } else {
                bool allValid = true;
                for (auto m : targets) {
                    bool valid = fastEval(*(v.query->first), *m);
                    if (!valid) {
                        allValid = false;
                        break;
                    }
                }
                if (allValid) {
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }
            }             
        } //All Next End

        //All Finally start
        else if(v.query->path == F){
            Edge *subquery = NULL;
            if (!v.query->first->isTemporal) {
                bool valid = fastEval(*(v.query->first), *(v.marking));
                if (valid) {
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }
            } else {
                subquery = new Edge(&v);
                Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
                subquery->targets.push_back(c);
            }
            
            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Edge* e1 = new Edge(&v);

                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e1->targets.push_back(c);
                }
                succ.push_back(e1);
            }

            if (subquery != NULL) {
                succ.push_back(subquery);
            }            

        }//All Finally end
    } //All end

    //Exists start
    else if (v.query->quantifier == E){

        //Exists Untill start
        if(v.query->path == U){
            Edge *right = NULL;
            if (v.query->second->isTemporal) {
                Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
                right = new Edge(&v);
                right->targets.push_back(c);
            } else {
                bool valid = fastEval(*(v.query->second), *(v.marking));
                if (valid) {
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }   // else: right condition is not satisfied, no need to add an edge
            }            

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Configuration *left = NULL;
                bool valid = false;
                if (v.query->first->isTemporal) {
                    left = createConfiguration(*(v.marking), *(v.query->first));
                } else {
                    valid = fastEval(*(v.query->first), *(v.marking));
                }
                if (left != NULL || valid) {
                    for(auto m : targets) {
                        Edge* e = new Edge(&v);
                        Configuration* c1 = createConfiguration(*m, *(v.query));
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
        else if(v.query->path == X){
            auto targets = nextState(*(v.marking));
            CTLTree* query = v.query->first;

            if(!targets.empty())
            {
                if (query->isTemporal) {    //have to check, no way to skip that
                    for(auto m : targets){                    
                        Edge* e = new Edge(&v);
                        Configuration* c = createConfiguration(*m, *query);
                        e->targets.push_back(c);
                        succ.push_back(e);
                    } 
                } else {
                    for(auto m : targets) {
                        bool valid = fastEval(*query, *m);
                        if (valid) {
                            succ.push_back(new Edge(&v));
                            v.Successors = succ;
                            return succ;
                        }   //else: It can't hold there, no need to create an edge
                    }
                }
            }
        }//Exists Next end

        //Exists Finally start
        else if(v.query->path == F){            
            Edge *subquery = NULL;
            if (!v.query->first->isTemporal) {
                bool valid = fastEval(*(v.query->first), *(v.marking));                
                if (valid) {
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }
            } else {
                Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
                subquery = new Edge(&v);
                subquery->targets.push_back(c);                
            }
        
            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c = createConfiguration(*m, *(v.query));
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
        //Edge* e = new Edge(&v);
        //e->targets.push_back(&v);
       // v.configPrinter();
        if (evaluateQuery(*v.query, *v.marking)){
            //assignConfiguration(&v, ONE);
            succ.push_back(new Edge(&v));
        } else {
            //assignConfiguration(&v, CZERO);
        }
        //succ.push_back(e);

    }

    v.Successors = succ;
    return succ;
    //computedSucc += succ.size();
    //std::cout << "-----------EDGES NOW : " << computedSucc << "\n" << std::flush;
}

Configuration &OnTheFlyDG::initialConfiguration()
{
    Markings.insert(&initial_marking);
    Configuration* initial = createConfiguration(initial_marking, *_query);
    return *initial;
}

bool OnTheFlyDG::evaluateQuery(CTLTree &query, Marking &marking){

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
                        //std::cout<<_net->placeNames()[query->a.fireset[fs_transition].denpencyplaces[dpc_place].placeLarger]<<" is true"<<std::endl;
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

    ///std::cout<<"Evaluating cardinality... "<<std::endl;
 //   t_config.configPrinter();
 //   std::cout<<"Less: ";
    int less = query.a.cardinality.intSmaller;
    int greater= query.a.cardinality.intLarger;
    if( less == -1 ){
        int i = 0;
        less = 0;
        for (i = 0; i < query.a.cardinality.placeSmaller.sizeoftokencount; i++){
            int index = query.a.cardinality.placeSmaller.cardinality[i];
            less += marking[index];
 //           std::cout<<t_config.marking->Value()[index]<<" + ";
        }
    }
//    std::cout<<" = "<<less<<std::endl;
//    std::cout<<"Greater: ";
    if (greater == -1){
        int i = 0;
        greater = 0;
       // std::cout<<"::: Number of places: "<<query->a.cardinality.placeLarger.sizeoftokencount<<std::endl;
        for (i = 0; i < query.a.cardinality.placeLarger.sizeoftokencount; i++){
            //std::cout<<"::::: i: "<<i<<std::endl;
            int index = query.a.cardinality.placeLarger.cardinality[i];
            //std::cout<<"::::: Index: "<<index<<" - Value: "<<t_config.marking->Value()[index]<<std::endl;
            greater += marking[index];
  //          std::cout<<t_config.marking->Value()[index]<<" + ";
       //     std::cout<<"::::: greater: "<<greater<<std::endl;
        }

    }
  //  std::cout<<" = "<<greater<<std::endl;

    result = less <= greater;
   // std::cout<<"... evaluation Done"<<std::endl;
    return result;
}

int OnTheFlyDG::indexOfPlace(char *t_place){
    for (int i = 0; i < _nplaces; i++) {
        if (0 == (strcmp(t_place, _petriNet->placeNames()[i].c_str()))){
                //cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                return i;
        }
    }
    return -1;
}

std::list<Marking*> OnTheFlyDG::nextState(Marking& t_marking){

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

std::list<int> OnTheFlyDG::calculateFireableTransistions(Marking &t_marking){

    std::list<int> fireableTransistions;
    
    for(int t = 0; t < _ntransitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < _nplaces; p++){
            if(t_marking[p] < _petriNet->inArc(p,t)){
                transitionFound = false;
                break;
            }
        }

        if(transitionFound){ //Inhibitor check
            for(auto inhibitor : _inhibitorArcs){
                if(inhibitor.weight > 0 && _petriNet->transitionNames()[t].compare(inhibitor.target) == 0 ){
                    for(int p = 0; p < _nplaces; p++){
                        if(_petriNet->placeNames()[p].compare(inhibitor.source) == 0 && t_marking[p] > inhibitor.weight){
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


void OnTheFlyDG::clear(bool t_clear_all)
{
//    int i = 0;
//    int max_i = Markings.size();
    for(Marking *m : Markings){
//        printf("Cleaning up marking %d of %d: ", ++i, max_i);
//        m->print();
//        printf("\n");

//        if(m == &initial_marking)
//            printf("Successor count: %d\n",(int) m->successors.size());
        for(Configuration *c : m->successors){
//            c->configPrinter();
            delete c;
        }

        m->successors.resize(0);
    }
//    printf("Done Cleaning Configurations\n");

//    for(Configuration *c : Configurations){
//        delete c;
//    }
//    Configurations.clear();

    if(t_clear_all){
        for(auto m : Markings){
            delete m;
        }
        Markings.clear();
    }
}

Configuration *OnTheFlyDG::createConfiguration(Marking &t_marking, CTLTree &t_query)
{
    for(Configuration* c : t_marking.successors){
        if(c->query == &t_query)
            return c;
    }

    Configuration* newConfig = new Configuration();
    newConfig->marking = &t_marking;
    newConfig->query = &t_query;

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->IsNegated = true;
    }

    t_marking.successors.push_back(newConfig);
    return newConfig;

    //auto pair = Configurations.insert(newConfig);

//    if(!pair.second) {
//        delete newConfig;
//    }

//    return *(pair.first);
}



Marking *OnTheFlyDG::createMarking(const Marking& t_marking, int t_transition){
    Marking* new_marking = new Marking();

    new_marking->CopyMarking(t_marking);

    for(int p = 0; p < _nplaces; p++){
        int place = (*new_marking)[p] - _petriNet->inArc(p,t_transition);
        (*new_marking)[p] = place + _petriNet->outArc(t_transition,p);
    }

    auto result = Markings.find(new_marking);

    if(result == Markings.end()){
    //   std::cout << "Inserted marking - Size now: " << Markings.size() << std::endl;
        return *(Markings.insert(new_marking).first);
    }
    else{
        delete new_marking;
    //    std::cout << "Marking exists - Size now: " << Markings.size() << std::endl;
    }


    return *result;
}
void OnTheFlyDG::initCompressOption(){
    _compressoption = true;
}
}//ctl
