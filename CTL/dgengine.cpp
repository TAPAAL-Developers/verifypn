#include "dgengine.h"
#include "../CTLParser/CTLParser.h"
#include "edgepicker.h"

#include <string.h>
#include <fstream>
#include <iostream>
#include <stack>
#include <queue>

namespace ctl {

DGEngine::DGEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]){
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
    _CZero = false;
}

void DGEngine::search(CTLTree *t_query, ctl_algorithm t_algorithm, ctl_search_strategy t_strategy){

    _strategy = t_strategy;

    Marking* firstMarking = new Marking(_m0, _nplaces);
    Configuration* v0 = createConfiguration(*firstMarking, *t_query);


    if(t_algorithm == Local && t_strategy != CTL_CDFS){
        //std::cout << "FORMULA:: Local Smolka with strategy:" << t_strategy << "\n";
        _querySatisfied = localSmolka(*v0);
        _querySatisfied = v0->assignment == ONE? true : false;
    }
    else if(t_algorithm == CZero){
        //std::cout << "FORMULA:: CZERO smolka with strategy:" << t_strategy << "\n";
        _CZero = true;
        _querySatisfied = localSmolka(*v0);
        _querySatisfied = v0->assignment == ONE? true : false;
        _CZero = false;
    }
    else if(t_algorithm == Global && t_strategy != CTL_CDFS){
        //std::cout << "FORMULA:: Global Smolka with strategy:" << t_strategy << "\n";
        buildDependencyGraph(*v0);
        _querySatisfied = globalSmolka(*v0);
        _querySatisfied = v0->assignment == ONE? true : false;
    }
    else if(t_algorithm != CZero && t_strategy == CTL_CDFS){
        std::cout << "Error algorithm and strategy mismatch.";
        return;
    }
    else{
        std::cout << "Error Unknown ctl algorithm" << std::endl;
        return;
    }
}

void DGEngine::clear(bool t_clear_all)
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

void DGEngine::buildDependencyGraph(Configuration &v){
    std::queue<Configuration*> C;
    v.assignment = ZERO;
    C.push(&v);

    //    Make dependency graph
    //    std::cout << "==========================================================" << std::endl;
    //    std::cout << "======================= Start Global =====================" << std::endl;
    //    std::cout << "==========================================================" << std::endl;
    while(!C.empty()){
        Configuration* c = C.front();
        C.pop();

        successors(*c);

        for(Edge* e : c->Successors){
 //           e->edgePrinter();
            for(Configuration* tc : e->targets){
                if(tc->assignment == UNKNOWN){
                    tc->assignment = ZERO;
                    C.push(tc);
                }
            }
        }
    }
}

void DGEngine::CalculateEdges(Configuration &v, EdgePicker &W){
    std::unordered_set< Configuration*,
                        std::hash<Configuration*>,
                        Configuration::Configuration_Equal_To> Visisted;

    std::queue<Configuration*> C;
    Visisted.insert(&v);
    C.push(&v);

    while(!C.empty()){
        Configuration* c = C.front();
        C.pop();
        for(Edge* e : c->Successors){
            W.push(e);
            for(Configuration* tc : e->targets){
                auto result = Visisted.insert(tc);
                if(result.second)
                    C.push(tc);
            }
        }
    }
}

bool DGEngine::globalSmolka(Configuration &v){
    EdgePicker W = EdgePicker(_strategy);
    //v.assignment = ZERO;

    CalculateEdges(v, W);
/*
    std::cout << "==========================================================" << std::endl;
    std::cout << "====== Traversing DG :: Size of W is: " << W.size() << std::endl;
    std::cout << "==========================================================" << std::endl;
*/
    while(!W.empty()){
        Edge* e = W.pop();

        //e->edgePrinter();
        if(e->source->assignment == ONE){}
        else if(e->source->IsNegated){
            globalSmolka(*(e->targets.front()));
            if(e->targets.front()->assignment == ONE)
                e->source->assignment = ZERO;
            else {
                e->source->assignment = ONE;
                for(auto de : e->source->DependencySet){
                    W.push_dependency(de);
                }
            }
        }
        else{
            bool allOnes = true;

            for(Configuration* tc : e->targets){
                if(tc->assignment == ZERO){
                    tc->DependencySet.push_back(e);
                    allOnes = false;
                }
            }

            if(allOnes){
                e->source->assignment = ONE;
                for(Edge* de : e->source->DependencySet){
                    W.push_dependency(de);
                }
                e->source->DependencySet.clear();
            }
        }
    }

   // std::cout << "Final value of v is: " << v.assignment << std::endl << std::flush;

    //Due to compiler optimization, this might return the wrong value
    return v.assignment == ONE ? true : false;
}

bool DGEngine::localSmolka(Configuration &v){
    v.assignment = ZERO;
    EdgePicker W = EdgePicker(_strategy);
    auto initialSucc = successors(v);

//    for(auto c : initialSucc){
//        c->edgePrinter();
//    }

    for(auto s : initialSucc) {
        W.push(s);
    }

    while (!W.empty()) {
    	//std::cout << "in w now, size of w:\n" << W.size() << std::flush;
    	//W.print();
		//std::cout << "NOT IN wnow:\n" << std::flush;
        int i = 0;  
        Edge* e = W.pop();
        e->processed = true;
     //  std::cout << "First edge :\n" << std::flush;
     //  e->edgePrinter();

/*       std::cout << "First edge  succ :\n" << std::flush;
       for (auto l : e->source->Successors){
       	l->edgePrinter();
       }*/

        /*****************************************************************/
        /*Data handling*/
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        int targetUKNOWNassignments = 0;
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
                targetUKNOWNassignments++;
            }

        }


        if(e->source->DependencySet.empty() && *e->source != v){
            //This is suppose to be empty.
            //If the D(e.source) is empty, no need to process it.
        }
        /******************************************************************/
        //Case: One
        else if (targetONEassignments == e->targets.size()){
            
//std::cout << "\n-----------SIZE OF E's TARGES :" << e->targets.size() << std::flush;
           // e->edgePrinter();
            #ifdef DEBUG
            cout<<"All assignments were ONE"<<endl;
            #endif


            //std::cout << "\nbefore assignment e " << e->source->assignment << std::flush;
            //std::cout << "before assignment v " << v.assignment << std::flush;

            assignConfiguration((e->source), ONE);
            
           // std::cout << "\nafter assignment e " << e->source->assignment << std::flush;
            //std::cout << "after assignment v " << v.assignment << std::flush;


            if(*(e->source) == v){

               /*std::cout << "------------first marking :"  << std::flush;   e->source->marking->print();

               std::cout << "\n------------second marking:"  << std::flush;   v.marking->print();*/
                
                   // W.reset();
                if(_strategy == CTL_BFS || _strategy == CTL_FBFS || _strategy == CTL_BBFS || _strategy == CTL_BestFS)
                { 
                    if(W.empty()){ return (e->source->assignment == ONE) ? true : false;}
               }else  return (e->source->assignment == ONE) ? true : false;
                
                
            }

            for(auto edge : e->source->DependencySet){
               //std::cout << "\n----------E's DS is :\n" << std::flush;
                //edge->edgePrinter();
                if(!(edge->source->assignment == ONE)){
                    W.push_dependency(edge);

                }

            }

            e->source->DependencySet.clear();
        }
        /*****************************************************************/
        // Case: CZERO
        else if(czero){
        	/*std::cout << "we are in czero with edge :\n" << std::flush;
        	e->edgePrinter();
        	std::cout << "where source dependency set :\n" << std::flush;
        	for (auto t : e->source->DependencySet){
        		t->edgePrinter();
        	}
        	std::cout << "and source successor set :\n" << std::flush;
        	for (auto t : e->source->Successors){
        		t->edgePrinter();
        	}
			

             /*bool isCzero = true;
             for(auto edge : e->source->Successors){
                 bool found = false;
                 for( auto c : edge->targets){
                     if(c->assignment == CZERO){
                         found = true;
                         break;
                     }
                 }
                 if(!found){
                     isCzero = false;
                     break;
                 }
             }

             if(isCzero){
                 e->source->assignment == CZERO;

                 for(auto edge : e->source->DependencySet)
                     W.push(edge);
                 e->source->DependencySet.clear();
             }*/


           if(e->source->Successors.size() == 1){

           	//std::cout << "begin\n" << std::flush;
           	//e->edgePrinter();
           	//std::cout << "DS\n" << std::flush;
               assignConfiguration((e->source), CZERO);

               if(*(e->source) == v)
                   return v.assignment == ONE ? true : false;

               for(auto edge : e->source->DependencySet){
              	//   edge->edgePrinter();
                   W.push_dependency(edge);
                }

               e->source->DependencySet.clear();
           }
           //std::cout << "and we are out" << std::flush;
           W.remove(e);
           e->source->removeSuccessor(e);

         // std::cout << "We now deleting :\n" << std::flush;

        }
        /*****************************************************************/
        // Case: Negated
        else if(e->source->IsNegated){


            Configuration* negConfig = *(e->targets.begin());
            localSmolka(*negConfig);
            negConfig->DependencySet.push_back(e);


            assignConfiguration((e->source), negConfig->assignment);

            if(e->source->assignment == ONE || e->source->assignment == CZERO){
                for(auto edge : e->source->DependencySet)   
                    W.push_dependency(edge);
            }
            e->source->DependencySet.clear();

            //std::cout << "\n---------- WE ARE DONE WITH REC SMOLKA------------\n" << std::flush;
        }
        /*****************************************************************/
        // CASE: ZERO
        else if ( targetZEROassignments > 0){
            for(auto c : e->targets){
                if(c->assignment == ZERO) {
                    //detectCircle(e->source, c);
                    c->DependencySet.push_back(e);
                }
            }
        }
        /*****************************************************************/
        // Case: UNKNOWN
        else if (targetUKNOWNassignments > 0){
        //    std::cout << "--------------SUCC------------------------------" << std::flush;

            for(auto c : e->targets){
                if(c->assignment == UNKNOWN){
                    c->assignment = ZERO;
                    c->DependencySet.push_back(e);
                    for(auto s : successors(*c)){
                //        s->edgePrinter();
                        W.push(s);
                    }
                }
            }
        }
        
    }
    //std::cout << "the final value is: " << v.assignment << "\n" << std::flush;
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
        Edge* e = new Edge(&v);
        e->targets.push_back(&v);
        if (evaluateQuery(v)){
            assignConfiguration(&v, ONE);
        } else {
            assignConfiguration(&v, CZERO);
        }
        succ.push_back(e);
        
    }

    v.Successors = succ;
    return succ;
    //computedSucc += succ.size();
    //std::cout << "-----------EDGES NOW : " << computedSucc << "\n" << std::flush;
}


bool DGEngine::evaluateQuery(Configuration &t_config){

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

int DGEngine::indexOfPlace(char *t_place){
    for (int i = 0; i < _nplaces; i++) {
        if (0 == (strcmp(t_place, _net->placeNames()[i].c_str()))){
                //cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                return i;
        }
    }
    return -1;
}

void DGEngine::assignConfiguration(Configuration* t_config, Assignment t_assignment){


    if(t_config->IsNegated){
        //Under zero means Assignment enum is either ZERO og CZERO
        if(t_assignment < 0){
            t_config->assignment = ONE;
        }
        else if(_CZero){
            t_config->assignment = CZERO;
        }
        else { t_config->assignment = ZERO; }
    }
    else {
        if(t_assignment > 0){
            t_config->assignment = t_assignment;
        }
        else if(_CZero){
            t_config->assignment = t_assignment;

        }
        else {
            t_config->assignment = ZERO;
        }
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
    
    auto result = Configurations.find(newConfig);
    if (result == Configurations.end()){
       // std::cout << "Inserted Configuration - Size now: " << Configurations.size() << std::endl;
        return *(Configurations.insert(newConfig).first);
    }
    //std::cout << "Configuration exists - Size now: " << Configurations.size() << std::endl;
    delete newConfig;
    return *result;
}

Marking* DGEngine::createMarking(const Marking& t_marking, int t_transition){
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

void DGEngine::colorprinter(std::string str, ColourCode cc){
    std::cout << "\033[" << cc << "m" << str << "\033[" << FG_DEFAULT << "m" << std::endl;
}

    void DGEngine::RunEgineTest(){
        //----------------------------------------------
        //----------- Test of Marking ------------------
        //----------------------------------------------
        //-------- New Marking - Start
        Marking *initmarking = new Marking(_m0, _nplaces);
        int i = 0;
        for (i = 0; i > _nplaces; i++){
            //Also Valid!!! assert((*initmarking)[i] == _m0[i]); 
            assert(initmarking->Value()[i] == _m0[i]);
        }
        //-------- New Marking - Done
        //-------- Create Marking - Start
        int t_r6;
        std::string t_r6_str = "r6";
        for (i = 0; i < _ntransitions; i++){
            const char *t_name = _net->transitionNames()[i].c_str();
            if (strcmp(t_name,t_r6_str.c_str()) == 0)
                t_r6 = i;
        }
        
        Marking *testmarking = createMarking(*initmarking, t_r6);
        for (i = 0; i < _nplaces; i++){
            if (i == 4 )
                assert(testmarking->Value()[i] == 1);
            else if (i == 7 || i == 8)
                assert(testmarking->Value()[i] == 0);
            else assert(testmarking->Value()[i] == _m0[i]);
        }
        //-------- Create Marking - Done
        //-------- Create Duplicate Marking - Start
        Marking *duplicatetestmarking = createMarking(*initmarking, t_r6);
        assert(duplicatetestmarking == testmarking);
        //-------- Create Duplicate Marking - Done
        
        //----------------------------------------------
        //-------------- Test Configuartion ------------
        //----------------------------------------------

        CTLParser *testparser = new CTLParser();
        CTLFormula *testquery[16];
        std::vector<char> simple_buffer = buffercreator(true, true);
        testparser->ParseXMLQuery(simple_buffer, testquery);
        Configuration *testinitconfig = createConfiguration(*(initmarking), *(testquery[0]->Query));
        
        CTLParser *Fireble_testparser = new CTLParser();
        CTLFormula *Fireble_testquery[16];
        std::vector<char> Fireble_buffer = buffercreator(true, false);
        Fireble_testparser->ParseXMLQuery(Fireble_buffer, Fireble_testquery);
        //Configuration *Fireble_testinitconfig = createConfiguration(*(initmarking), *(Fireble_testquery[0]->Query));
        
        CTLParser *Cardi_testparser = new CTLParser();
        CTLFormula *Cardi_testquery[16];
        std::vector<char> Cardi_buffer = buffercreator(false, false);
        Cardi_testparser->ParseXMLQuery(Cardi_buffer, Cardi_testquery);
        //Configuration *Cardi_testinitconfig = createConfiguration(*(initmarking), *(Cardi_testquery[0]->Query));

        //-------- Create  Config - Start
        for (i = 0; i > _nplaces; i++){
            assert(testinitconfig->marking->Value()[i] == _m0[i]);
        }
        assert(testinitconfig->query == testquery[0]->Query);
        //-------- Create  Config - Done

        //-------- Create Duplicate Config - Start
        testinitconfig->assignment = ONE;
        Configuration *duplicatetestconfig = createConfiguration(*(initmarking), *(testquery[0]->Query));
        assert(duplicatetestconfig == testinitconfig);
        assert(duplicatetestconfig->assignment == ONE);
        //-------- Create Duplicate Config - Done
        

        //----------------------------------------------
        //--------------- Test Fireability -------------
        //----------------------------------------------
        //-------- Create fireability list - Start
        std::list<int> possible_t_list = calculateFireableTransistions(*initmarking);
        assert(possible_t_list.size() == 2);
        assert(possible_t_list.front() == 0);
        assert(possible_t_list.back() == 4);
        //-------- Create fireability list - Done
        

        //----------------------------------------------
        //--------------- Test Next state -------------
        //----------------------------------------------
        //-------- Create next state list - Start
        std::list<Marking*> next_state_list = nextState(*initmarking);
        std::list<Marking*>::iterator ns_iter = next_state_list.begin();
        
        
        assert(next_state_list.size() == 2);
        for (i = 0; i < _nplaces; i++){
            if (i == 0 || i == 1)
                assert(next_state_list.front()->Value()[i] == 0);
            else if (i == 2)
                assert(next_state_list.front()->Value()[i] == 1);
            else assert(next_state_list.front()->Value()[i] == _m0[i]);
        }
        for (i = 0; i < _nplaces; i++){
            if (i == 4 )
                assert(next_state_list.back()->Value()[i] == 1);
            else if (i == 7 || i == 8)
                assert(next_state_list.back()->Value()[i] == 0);
            else assert(next_state_list.back()->Value()[i] == _m0[i]);
        }
        //-------- Create next state list - Done

        
        //----------------------------------------------
        //----------- Test of assignment ------------------
        //----------------------------------------------
        //-------- Unnegated assignment confirmation - Start
        testinitconfig->assignment = UNKNOWN;
        assert(testinitconfig->assignment == UNKNOWN);
        
        assignConfiguration(testinitconfig, ONE);
        assert(testinitconfig->assignment == ONE);
        
        assignConfiguration(testinitconfig, ZERO);
        assert(testinitconfig->assignment == ZERO);
        
        assignConfiguration(testinitconfig, CZERO);
        if (_CZero)
            assert(testinitconfig->assignment == CZERO);
        else {
            assert(testinitconfig->assignment == ZERO);
        }
        //-------- Unnegated assignment confirmation - Done
        //-------- Negated assignment confirmation - Start
        Configuration *negatedconfig = createConfiguration(*(initmarking), *(testquery[11]->Query));
        negatedconfig->assignment = UNKNOWN;
        assert(negatedconfig->assignment == UNKNOWN);
        
        negatedconfig->configPrinter();
        
        assignConfiguration(negatedconfig, ONE);
        if (_CZero)
            assert(negatedconfig->assignment == CZERO);
        else assert(negatedconfig->assignment == ZERO);
        
        assignConfiguration(negatedconfig, ZERO);
        assert(negatedconfig->assignment == ONE);
        
        assignConfiguration(negatedconfig, CZERO);
        if (_CZero)
            assert(negatedconfig->assignment == ONE);
        else assert(negatedconfig->assignment == ONE);
        //-------- Negated assignment confirmation - Done
        
        //----------------------------------------------
        //----------- Test of indexOfPlace() -----------
        //----------------------------------------------
        //-------- Check false place - Start
        std::string false_placename_str = "SOmeR4nd0MMM-nezz";
        size_t size = false_placename_str.size();
        char* false_placename = strcpy((char*)malloc(sizeof(char)*size), false_placename_str.c_str());
        assert(-1 == indexOfPlace(false_placename));
        //-------- Check false place - Done
        //-------- Check true place - Start
        std::string true_placename_str = "Raf1Star";
        size_t t_size = true_placename_str.size();
        char* true_placename = strcpy((char*)malloc(sizeof(char)*t_size), true_placename_str.c_str());
        assert(0 == indexOfPlace(true_placename));
        //-------- Check true place - Done
        
        //----------------------------------------------
        //------- Test of evaluateQuery(v):bool  -------
        //----------------------------------------------
        //Configuration *Simple_testinitconfig = createConfiguration(*(initmarking), *(testquery[0]->Query));
        //Configuration *Fireble_testinitconfig = createConfiguration(*(initmarking), *(Fireble_testquery[0]->Query));
        //Configuration *Cardi_testinitconfig = createConfiguration(*(initmarking), *(Cardi_testquery[0]->Query));
        //-------- Evaluate single isFire - Start
        
        //-------- Evaluate single isFire - Done
        //-------- Evaluate Multi isFire - Start
        //-------- Evaluate Multi isFire - Done
        //-------- Evaluate int vs place - Start
        //-------- Evaluate int vs place - Done
        //-------- Evaluate place vs place - Start
        //-------- Evaluate place vs place - Done
        //-------- Evaluate place vs int - Start
        //-------- Evaluate place vs int - Done
        
    }
    
    std::vector<char> DGEngine::buffercreator(bool fire, bool simple){
        CTLFormula *queryList[8];
        std::string queryXMLlist[8];
        std::string querypath;
        
        //"testFramework/ModelDB/ERK-PT-000001/";
        std::string querypath_simple = "testFramework/ModelDB/ERK-PT-000001/CTLFireabilitySimple.xml";
        std::string querypath_fireable = "testFramework/ModelDB/ERK-PT-000001/CTLFireability.xml";
        std::string querypath_cardinality = "testFramework/ModelDB/ERK-PT-000001/CTLCardinality.xml";
        
        fire ? ( simple ? querypath = querypath_simple : querypath = querypath_fireable) : querypath = querypath_cardinality; 
        
        const char* queryfile = querypath.c_str();
        std::ifstream xmlfile (queryfile);
        std::vector<char> buffer((std::istreambuf_iterator<char>(xmlfile)), std::istreambuf_iterator<char>());
        buffer.push_back('\0');
        return buffer;
    }
}
