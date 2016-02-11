#include "OnTheFlyDG.h"

#include <string.h>

namespace ctl{

OnTheFlyDG::OnTheFlyDG(PetriEngine::PetriNet *t_net, PetriEngine::MarkVal *t_initial):
    DependencyGraph(t_net, t_initial){}

std::list<Edge *> OnTheFlyDG::successors(Configuration &v)
{
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
        //Edge* e = new Edge(&v);
        //e->targets.push_back(&v);
       // v.configPrinter();
        if (evaluateQuery(v)){
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
    Marking *initial = new Marking(_initialMarking, _nplaces);
    return *createConfiguration(*initial, *_query);
}

bool OnTheFlyDG::evaluateQuery(Configuration &t_config){

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

    ///std::cout<<"Evaluating cardinality... "<<std::endl;
 //   t_config.configPrinter();
 //   std::cout<<"Less: ";
    int less = query->a.cardinality.intSmaller;
    int greater= query->a.cardinality.intLarger;
    if( less == -1 ){
        int i = 0;
        less = 0;
        for (i = 0; i < query->a.cardinality.placeSmaller.sizeoftokencount; i++){
            int index = query->a.cardinality.placeSmaller.cardinality[i];
            less += t_config.marking->Value()[index];
 //           std::cout<<t_config.marking->Value()[index]<<" + ";
        }
    }
//    std::cout<<" = "<<less<<std::endl;
//    std::cout<<"Greater: ";
    if (greater == -1){
        int i = 0;
        greater = 0;
       // std::cout<<"::: Number of places: "<<query->a.cardinality.placeLarger.sizeoftokencount<<std::endl;
        for (i = 0; i < query->a.cardinality.placeLarger.sizeoftokencount; i++){
            //std::cout<<"::::: i: "<<i<<std::endl;
            int index = query->a.cardinality.placeLarger.cardinality[i];
            //std::cout<<"::::: Index: "<<index<<" - Value: "<<t_config.marking->Value()[index]<<std::endl;
            greater += t_config.marking->Value()[index];
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
            if(t_marking[p] < _petriNet->inArc(p,t))
                transitionFound = false;
        }

        if(transitionFound)
            fireableTransistions.push_back(t);
    }
    return fireableTransistions;
}


void OnTheFlyDG::clear(bool t_clear_all)
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

Configuration *OnTheFlyDG::createConfiguration(Marking &t_marking, CTLTree &t_query)
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
}//ctl
