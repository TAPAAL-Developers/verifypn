#include "OnTheFlyDG.h"

#include <string.h>
#include <algorithm>

namespace ctl{

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net, PetriEngine::MarkVal *t_initial, PNMLParser::InhibitorArcList inhibitorArcs):
    DependencyGraph(t_net, t_initial, inhibitorArcs)
{
    _compressoption = false;
    cached_successors.resize(t_net->numberOfTransitions());
}


bool OnTheFlyDG::fastEval(CTLQuery &query, Marking &marking) {
    assert(!query.IsTemporal);
    if (query.GetQuantifier() == AND){
        return fastEval(*query.GetFirstChild(), marking) && fastEval(*query.GetSecondChild(), marking);        
    } else if (query.GetQuantifier() == OR){
        return fastEval(*query.GetFirstChild(), marking) || fastEval(*query.GetSecondChild(), marking);
    } else if (query.GetQuantifier() == NEG){
        bool result = fastEval(*query.GetFirstChild(), marking);
        std::cout << "fastevaled to: " << std::boolalpha << result << " negating to " << !result << std::endl;
        return !result;
    } else {   
        bool res = evaluateQuery(query, marking);
        if(res)
            std::cout<<" TRUE"<<std::endl;
        else
            std::cout<<" FALSE"<<std::endl;
        return res;
    }
}

std::list<Edge *> OnTheFlyDG::successors(Configuration &v)
{
    std::list<Edge*> succ;
    CTLType query_type = v.query->GetQueryType();
    if(query_type == EVAL){
        //assert(false && "Someone told me, this was a bad place to be.");
        if (evaluateQuery(*v.query, *v.marking)){
            succ.push_back(new Edge(&v));
        }
    }
    else if (query_type == LOPERATOR){
        if(v.query->GetQuantifier() == NEG){
            std::cout<<"Found NEG query"<<std::endl;
            Configuration* c = createConfiguration(*(v.marking), *(v.query->GetFirstChild()));
            std::cout<<"IsNegated: "<< std::boolalpha << c->IsNegated << std::endl;
            Edge* e = new Edge(&v);
            e->targets.push_back(c);
            succ.push_back(e);
        }
        else if(v.query->GetQuantifier() == AND){
            //Check if left is false
            if(!v.query->GetFirstChild()->IsTemporal){
                if(!fastEval(*(v.query->GetFirstChild()), *v.marking))
                    //query cannot be satisfied, return empty succ set
                    return succ;
            }

            //check if right is false
            if(!v.query->GetSecondChild()->IsTemporal){
                if(!fastEval(*(v.query->GetSecondChild()), *v.marking))
                    return succ;
            }

            Edge *e = new Edge(&v);

            //If we get here, then either both propositions are true(should not be possible)
            //Or a temporal operator and a true proposition
            //Or both are temporal
            if(v.query->GetFirstChild()->IsTemporal){
                e->targets.push_back(createConfiguration(*v.marking, *(v.query->GetFirstChild())));
            }
            if(v.query->GetSecondChild()->IsTemporal){
                e->targets.push_back(createConfiguration(*v.marking, *(v.query->GetSecondChild())));
            }
            succ.push_back(e);
        }
        else if(v.query->GetQuantifier() == OR){
            //Check if left is true
            if(!v.query->GetFirstChild()->IsTemporal){
                if(fastEval(*(v.query->GetFirstChild()), *v.marking)){
                    //query is satisfied, return
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }
            }

            if(!v.query->GetSecondChild()->IsTemporal){
                if(fastEval(*(v.query->GetSecondChild()), *v.marking)){
                    succ.push_back(new Edge(&v));
                    v.Successors = succ;
                    return succ;
                }
            }

            //If we get here, either both propositions are false
            //Or one is false and one is temporal
            //Or both temporal
            if(v.query->GetFirstChild()->IsTemporal){
                Edge *e = new Edge(&v);
                e->targets.push_back(createConfiguration(*v.marking, *(v.query->GetFirstChild())));
                succ.push_back(e);
            }
            if(v.query->GetSecondChild()->IsTemporal){
                Edge *e = new Edge(&v);
                e->targets.push_back(createConfiguration(*v.marking, *(v.query->GetSecondChild())));
                succ.push_back(e);
            }
        }
        else{
            assert(false && "An unknown error occoured in the loperator-part of the successor generator");
        }
    }
    else if (query_type == PATHQEURY){
        if(v.query->GetQuantifier() == A){
            if (v.query->GetPath() == U){
                Edge *right = NULL;
                if (!v.query->GetSecondChild()->IsTemporal){
                    //right side is not temporal, eval it right now!
                    bool valid = fastEval(*(v.query->GetSecondChild()), *(v.marking));
                    if (valid) {    //satisfied, no need to go through successors
                        succ.push_back(new Edge(&v));
                        v.Successors = succ;
                        return succ;
                    }//else: It's not valid, no need to add any edge, just add successors
                }
                else {
                    //right side is temporal, we need to evaluate it as normal
                    Configuration* c = createConfiguration(*(v.marking), *(v.query->GetSecondChild()));
                    right = new Edge(&v);
                    right->targets.push_back(c);
                }
                bool valid = false;
                Configuration *left = NULL;
                if (!v.query->GetFirstChild()->IsTemporal) {
                    //left side is not temporal, eval it right now!
                    valid = fastEval(*(v.query->GetFirstChild()), *(v.marking));
                } else {
                    //left side is temporal, include it in the edge
                    left = createConfiguration(*(v.marking), *(v.query->GetFirstChild()));
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
            }
            else if(v.query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v.query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v.query->GetFirstChild()), *(v.marking));
                    if (valid) {
                        succ.push_back(new Edge(&v));
                        v.Successors = succ;
                        return succ;
                    }
                } else {
                    subquery = new Edge(&v);
                    Configuration* c = createConfiguration(*(v.marking), *(v.query->GetFirstChild()));
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
            }
            else if(v.query->GetPath() == X){
                auto targets = nextState(*v.marking);
                if (v.query->GetFirstChild()->IsTemporal) {   //regular check
                    Edge* e = new Edge(&v);
                    for (auto m : targets){
                        Configuration* c = createConfiguration(*m, *(v.query->GetFirstChild()));
                        e->targets.push_back(c);
                    }
                    succ.push_back(e);
                } else {
                    bool allValid = true;
                    for (auto m : targets) {
                        bool valid = fastEval(*(v.query->GetFirstChild()), *m);
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
            }
            else if(v.query->GetPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
        else if(v.query->GetQuantifier() == E){
            if (v.query->GetPath() == U){
                Edge *right = NULL;
                if (v.query->GetSecondChild()->IsTemporal) {
                    Configuration* c = createConfiguration(*(v.marking), *(v.query->GetSecondChild()));
                    right = new Edge(&v);
                    right->targets.push_back(c);
                } else {
                    bool valid = fastEval(*(v.query->GetSecondChild()), *(v.marking));
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
                    if (v.query->GetFirstChild()->IsTemporal) {
                        left = createConfiguration(*(v.marking), *(v.query->GetFirstChild()));
                    } else {
                        valid = fastEval(*(v.query->GetFirstChild()), *(v.marking));
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
            }
            else if(v.query->GetPath() == F){
                Edge *subquery = NULL;
                if (!v.query->GetFirstChild()->IsTemporal) {
                    bool valid = fastEval(*(v.query->GetFirstChild()), *(v.marking));                
                    if (valid) {
                        succ.push_back(new Edge(&v));
                        v.Successors = succ;
                        return succ;
                    }
                } else {
                    Configuration* c = createConfiguration(*(v.marking), *(v.query->GetFirstChild()));
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
            }
            else if(v.query->GetPath() == X){
                auto targets = nextState(*(v.marking));
                CTLQuery* query = v.query->GetFirstChild();

                if(!targets.empty())
                {
                    if (query->IsTemporal) {    //have to check, no way to skip that
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
            }
            else if(v.query->GetPath() == G ){
                assert(false && "Path operator G had not been translated - Parse error detected in succ()");
            }
            else
                assert(false && "An unknown error occoured in the successor generator");
        }
        
    }
    
    v.Successors = succ;
    return succ;
}

Configuration &OnTheFlyDG::initialConfiguration()
{
    Markings.insert(&initial_marking);
    Configuration* initial = createConfiguration(initial_marking, *_query);
    return *initial;
}

bool OnTheFlyDG::evaluateQuery(CTLQuery &query, Marking &marking){
    assert(query.GetQueryType() == EVAL);
    EvaluateableProposition *proposition = query.GetProposition();

    std::cout << "proposition type: " << (proposition->GetPropositionType() == FIREABILITY ? "Fireability" : "Cardinality") << std::endl;

    if (proposition->GetPropositionType() == FIREABILITY) {
        std::list<int> transistions = calculateFireableTransistions(marking);
        std::list<int>::iterator it;
        transistions.sort();

        for(const auto f : proposition->GetFireset()){
            it = std::find(transistions.begin(), transistions.end(), f);
            if(it != transistions.end()){
                return true;
            }
        }
        return false;
    }
    else if (proposition->GetPropositionType() == CARDINALITY){
        int first_param = GetParamValue(proposition->GetFirstParameter(), marking);
        int second_param = GetParamValue(proposition->GetSecondParameter(), marking);
        std::cout<<"Eval: "<<first_param<<" ? "<<second_param;
        return EvalCardianlity(first_param, proposition->GetLoperator(), second_param);
    }
    else
        assert(false && "Incorrect query proposition type was attempted evaluated");

}

int OnTheFlyDG::GetParamValue(CardinalityParameter *param, Marking& marking) {

    std::cout << "CarParam - isPlace: " << std::boolalpha << param->isPlace << " value: " << param->value << std::endl;

    if(param->isPlace){
        return marking[param->value];
    }
    else
        return param->value;
}

bool OnTheFlyDG::EvalCardianlity(int a, LoperatorType lop, int b) {
    if(lop == EQ)
        return a == b;
    else if (lop == LE)
        return a < b;
    else if (lop == LEQ)
        return a <= b;
    else if (lop == GR)
        return a > b;
    else if (lop == GRQ)
        return a >= b;
    assert(false && "Unsupported LOperator attemped evaluated");
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

Configuration *OnTheFlyDG::createConfiguration(Marking &t_marking, CTLQuery &t_query)
{
    for(Configuration* c : t_marking.successors){
        if(c->query == &t_query)
            return c;
    }

    Configuration* newConfig = new Configuration();
    newConfig->marking = &t_marking;
    newConfig->query = &t_query;

    //Default value is false
    if(t_query.GetQueryType() == LOPERATOR && t_query.GetQuantifier() == NEG){
        std::cout<<"HAH! I detected it"<<std::endl;
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
