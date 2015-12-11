#include "czero_fp_algorithm.h"

#include "string.h"

namespace ctl{

CZero_FP_Algorithm::CZero_FP_Algorithm(PetriEngine::PetriNet *net, PetriEngine::MarkVal *initialmarking)
{
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

bool CZero_FP_Algorithm::search(CTLTree *t_query, EdgePicker *t_W)
{
    Marking *m0 = new Marking(_m0, _nplaces);
    Configuration *c0 = createConfiguration(*m0, *t_query);
    return czero_fp_algorithm(*c0);
}

bool CZero_FP_Algorithm::czero_fp_algorithm(Configuration &v)
{
    return false;
}

std::list<Edge*> CZero_FP_Algorithm::successors(Configuration& v) {
    std::list<Edge*> succ;
    //All
    if(v.query->quantifier == A){
        //All Until
        if(v.query->path == U){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);

            auto targets = nextState (*(v.marking));

            if(!targets.empty()){
                Edge* e1 = new Edge(&v);
                Configuration* b = createConfiguration(*(v.marking), *(v.query->first));

                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e1->targets.push_back(c);
                }
                e1->targets.push_back(b);
                succ.push_back(e1);
            }

            succ.push_back(e);

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
                succ.push_back(e);
            }
        } //All Next End

        //All Finally start
        else if(v.query->path == F){
            Edge* e = new Edge(&v);
            Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
            e->targets.push_back(c);

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Edge* e1 = new Edge();
                e1->source = &v;

                for(auto m : targets){
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e1->targets.push_back(c);
                }
                succ.push_back(e1);
            }
            succ.push_back(e);
        }//All Finally end
    } //All end

    //Exists start
    else if (v.query->quantifier == E){

        //Exists Untill start
        if(v.query->path == U){
            Configuration* c = createConfiguration(*(v.marking), *(v.query->second));
            Edge* e = new Edge(&v);
            e->targets.push_back(c);

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                Configuration* c = createConfiguration(*(v.marking), *(v.query->first));
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c1 = createConfiguration(*m, *(v.query));
                    e->targets.push_back(c1);
                    e->targets.push_back(c);
                    succ.push_back(e);
                }
            }
            succ.push_back(e);
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

            auto targets = nextState(*(v.marking));

            if(!targets.empty()){
                for(auto m : targets){
                    Edge* e = new Edge(&v);
                    Configuration* c = createConfiguration(*m, *(v.query));
                    e->targets.push_back(c);
                    succ.push_back(e);
                }
            }
            succ.push_back(e);

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
        if (evaluateQuery(v)){
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


bool CZero_FP_Algorithm::evaluateQuery(Configuration &t_config){

    CTLTree* query = t_config.query;

    bool result = false;

    if (t_config.query->a.isFireable) {
        std::list<int> transistions = calculateFireableTransistions(*(t_config.marking));

        for (auto t : transistions) {
            int fs_transition = 0;
            for(fs_transition = 0; fs_transition < query->a.firesize; fs_transition++){
                int dpc_place = 0;
                int truedependencyplaces = 0;
                for (dpc_place = 0; dpc_place < query->a.fireset[fs_transition].sizeofdenpencyplaces; dpc_place++){

                    if((query->a.fireset[fs_transition].denpencyplaces[dpc_place].intSmaller - 1) < t_config.marking->Value()[query->a.fireset[fs_transition].denpencyplaces[dpc_place].placeLarger]){
                        //std::cout<<_net->placeNames()[query->a.fireset[fs_transition].denpencyplaces[dpc_place].placeLarger]<<" is true"<<std::endl;
                        truedependencyplaces++;
                    }
                }
                if (truedependencyplaces == query->a.fireset[fs_transition].sizeofdenpencyplaces){
                    result = true;
                }
            }
        }
        return result;
    }

    int less = query->a.tokenCount.intSmaller;
    int greater= query->a.tokenCount.intLarger;

    if( less == -1 ){
        int index = query->a.tokenCount.placeSmaller;
        less = t_config.marking->Value()[index];
    }

    if (greater == -1){
        int index = query->a.tokenCount.placeLarger;
        greater = t_config.marking->Value()[index];
    }

    result = less <= greater;
    //std::cout << "Evaluation: " << result << std::endl << std::flush;

    return result;
}

int CZero_FP_Algorithm::indexOfPlace(char *t_place){
    for (int i = 0; i < _nplaces; i++) {
        if (0 == (strcmp(t_place, _net->placeNames()[i].c_str()))){
                //cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                return i;
        }
    }
    return -1;
}

std::list<Marking*> CZero_FP_Algorithm::nextState(Marking& t_marking){

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

std::list<int> CZero_FP_Algorithm::calculateFireableTransistions(Marking &t_marking){

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

void CZero_FP_Algorithm::clear(bool t_clear_all)
{
    for(auto c : Configurations){
        delete c;
    }
    Configurations.clear();

    if(t_clear_all){
        for(auto m : Markings){
            delete m;
        }
        Markings.clear();
    }
}

Configuration *CZero_FP_Algorithm::createConfiguration(Marking &t_marking, CTLTree &t_query)
{
    Configuration* newConfig = new Configuration();

    newConfig->marking = &t_marking;
    newConfig->query = &t_query;

    //Default value is false
    if(t_query.quantifier == NEG){
        newConfig->IsNegated = true;
    }

    auto pair = Configurations.insert(newConfig);

    if(!pair.second) {
        delete newConfig;
    }

    return *(pair.first);
}



Marking *CZero_FP_Algorithm::createMarking(const Marking& t_marking, int t_transition){
    Marking* new_marking = new Marking();

    new_marking->CopyMarking(t_marking);

    for(int p = 0; p < _nplaces; p++){
        int place = (*new_marking)[p] - _net->inArc(p,t_transition);
        (*new_marking)[p] = place + _net->outArc(t_transition,p);
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
}//ctl
