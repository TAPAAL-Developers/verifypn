/* 
 * File:   CTLEngine.cpp
 * Author: mossns
 * 
 * Created on September 11, 2015, 9:05 AM
 */

#include <PetriParse/PNMLParser.h>

#include <stdio.h>
#include <PetriEngine/PetriNetBuilder.h>
#include <PetriEngine/PQL/PQL.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assert.h>

//Unordered set area
#include <stdlib.h>
#include <algorithm>
//#include <unordered_set>

#include <PetriEngine/PQL/PQLParser.h>
#include <PetriEngine/PQL/Contexts.h>

#include <PetriEngine/Reachability/LinearOverApprox.h>
#include <PetriEngine/Reachability/UltimateSearch.h>
#include <PetriEngine/Reachability/RandomDFS.h>
#include <PetriEngine/Reachability/DepthFirstReachabilitySearch.h>
#include <PetriEngine/Reachability/BreadthFirstReachabilitySearch.h>

#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"

#include "CTLParser/CTLParser.h"
#include "CTLEngine.h"



CTLEngine::CTLEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool CertainZero) {
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
    querySatisfied = false;
    _CertainZero = CertainZero; 

}

CTLEngine::CTLEngine(const CTLEngine& orig) {
}

CTLEngine::~CTLEngine() {
}


//Public functions
void CTLEngine::search(CTLTree *query){
    //pNetPrinter(_net, _m0);
    #ifdef DEBUG
    cout << "--------------------- NEW QUERY-------------------------------------------\n" << flush;
    #endif
    Configuration v0 = createConfiguration(_m0, query);
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
    uset.clear();
    querySatisfied = localSmolka(v0);
}
bool CTLEngine::readSatisfactory() {
    return querySatisfied;
}






//Private functions

bool CTLEngine::localSmolka(Configuration v){
    *(v.assignment) = ZERO;
    //cout << "FIRST config :\n" << flush;
   // configPrinter(v);
    std::vector<CTLEngine::Edge> W;
    successors(v,W);
    while (W.size() != 0) {
        #ifdef DEBUG
        cout << "Starting while loop - size of W:" << W.size() <<endl;
        cout << "--------- NUMBER OF EDGES IN w NOW AND THEIR LOOK "<< W.size() << "\n" << flush;
        for(int k = 0; k < W.size(); k++){
            edgePrinter(W.at(k));
        }
        #endif
        int i = 0;
        Edge e = W.back();
        W.pop_back();
        /*****************************************************************/
        /*Data handling*/
        #ifdef DEBUG
        cout<<"Starting Data Handling\n"<<flush;
        #endif
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        int targetCZEROassignments = 0;
        int targetUNKNOWNassignments = 0;

        if(_CertainZero){
        	if(calculateCZERO(e, W)) targetCZEROassignments = 1;
        }



        for (i = 0; i < e.targets.size(); i++ ){
            #ifdef DEBUG
            cout<<"Target "<< i << " out of " << e.targets.size() << " assignment: "<< *(e.targets[i].assignment) << "\n"<<flush;
            #endif
            if (*(e.targets[i].assignment) == ONE) {
                targetONEassignments++;
            }
            else if (*(e.targets[i].assignment) == ZERO) {
                targetZEROassignments++;
            }
            else if (*(e.targets[i].assignment) == UNKNOWN) {
            	targetUNKNOWNassignments++;
            }
        }
        #ifdef DEBUG
        cout<<"Completed Data Handling\nResult:\n"<<flush;
        cout<<"ONE's: "<< targetONEassignments<<"\n" <<flush;
        cout<<"Zero's: "<< targetZEROassignments<<"\n" <<flush;
        cout<<"CZERO's: "<< targetCZEROassignments<<"\n" <<flush;
        cout<<"Unknowns's: "<< targetUNKNOWNassignments<<"\n" <<flush;
        #endif
        /*****************************************************************/
        /*if A(u) = 1, ∀u ∈ T then A(v) ← 1; W ← W ∪ D(v);*/
        if (targetONEassignments == e.targets.size()) {
            #ifdef DEBUG
            cout<<"All targets were 1-assigned\n"<<flush;
            #endif

            assignConfiguration(e.source, ONE);

            if(e.source == v){
			    return (*(v.assignment) == ONE) ? true : false;
			}
		

            W.insert(W.end(), e.source.denpendencyList.begin(), e.source.denpendencyList.end());

            /*cout << "Sdependency set - for:\n" << flush;
	        edgePrinter(e);
	        cout << "--------- NUMBER OF EDGES IN D NOW AND THEIR LOOK "<< e.source.denpendencyList.size() << "\n" << flush;
	        for(auto it = e.source.denpendencyList.begin(); it != e.source.denpendencyList.end(); ++it){
	            edgePrinter(*it);
	        }*/
                
                #ifdef DEBUG
                cout << "\n\n\n\n assigning to one \n\n\n\n" << flush;
                #endif
            
        }
        if (targetCZEROassignments > 0) {
            
            assignConfiguration(e.source, CZERO);

            if(e.source == v){
			    	return (*(v.assignment) == ONE) ? true : false;
            } 

            W.insert(W.end(), e.source.denpendencyList.begin(), e.source.denpendencyList.end());
                


                #ifdef DEBUG
                cout << "\n\n\n\n assigning to certain zero\n\n\n\n" << flush;
                #endif
        }
        
        /*****************************************************************/ 
        /*else if ∃u ∈ T where A(u) = 0 then D(u) ← D(u) ∪ {e}*/
        if (targetZEROassignments > 0) {
            #ifdef DEBUG
            cout<<"One or more targets were 0-assigned\n"<<flush;
            #endif

            for (i = 0; i < e.targets.size(); i++ ){
                if (*(e.targets[i].assignment) == ZERO)
                    e.targets[i].denpendencyList.push_back(e);
            }
        }
        /*****************************************************************/ 
        /*else if ∃u ∈ T where A(u) = ⊥ then A(u) ← 0; D(u) ← D(u) ∪ e; W ← W ∪ succ(u)*/
        if (targetUNKNOWNassignments > 0)  {
            #ifdef DEBUG
            cout<<"All assignments were unknown"<<endl;
            #endif
            for (i = 0; i < e.targets.size(); i++ ){
                if (*(e.targets[i].assignment) == UNKNOWN) {
                    Configuration u = e.targets[i];
                    assignConfiguration(u, ZERO);
                    u.denpendencyList.push_back(e);
                    successors(u,W);
                   
                    //#ifdef PP
                    /*cout << "currnet config: \n" << flush;                
                    configPrinter(u);
                    cout << "current dependency for this is\n " << flush;
                    edgePrinter(u.denpendencyList.back()); */
                    //#endif
                    #ifdef DEBUG
                    cout << "\n\n\n\n assigning to zero \n\n\n\n" << flush;
                    #endif
                }
            }
        }
        /*****************************************************************/ 
    }
    #ifdef DEBUG
    cout<<"The final assignment of the initial configuration is: " << *(v.assignment)<<endl;
    #endif
    //cout << "the final value is: " << *(v.assignment) << "\n" << flush;
    //assignConfiguration(v, *(v.assignment));
    return (*(v.assignment) == ONE) ? true : false;
}




void CTLEngine::assignConfiguration(Configuration v, Assignment a) {
	if(v.shouldBeNegated == true && a == ONE){
		if(_CertainZero){ *(v.assignment) = CZERO; }
		else {*(v.assignment) = ZERO;}
	} else if(v.shouldBeNegated == true && (a == CZERO || a == ZERO)) {
		*(v.assignment) = ONE;
	} else if(a == CZERO && !(_CertainZero)) {
		*(v.assignment) = ZERO;
	} else {
		*(v.assignment) = a;
	}
}



void CTLEngine::successors(Configuration v, std::vector<CTLEngine::Edge>& W) {
	if(v.query->quantifier == A){
		if(v.query->path == U){
	        Configuration c = createConfiguration(v.marking, v.query->second);
	        Edge e, e1;
	        e.source = v;
	        e.targets.push_back(c);
	        W.push_back(e);

	        e1.source = v;
	        Configuration b = createConfiguration(v.marking, v.query->first);
	        e1.targets.push_back(b);
	        int i = 0;
	        while(true){             
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
	            	i++;
	            Configuration c1 = createConfiguration(nxt_m, v.query);
	            e1.targets.push_back(c1);          

	            } else break;
	        }
            #ifdef PP
            edgePrinter(e);edgePrinter(e1);
            #endif
            if(i > 0){
             W.push_back(e1);             
            }
            

	    } else if(v.query->path == X){
            bool nxt_s = false;
	    	Edge e;
	    	e.source = v;
	    	while(true){             
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
                nxt_s = true;
	            Configuration c1 = createConfiguration(nxt_m, v.query->first);
	            e.targets.push_back(c1);          

	            } else break;
	        }
            if(nxt_s){ W.push_back(e);}
	    } else if(v.query->path == F){
	    	Configuration c = createConfiguration(v.marking, v.query->first);
            bool nxt_s = false;
	    	Edge e, e1;
	    	e.source = v;
	    	e.targets.push_back(c);

    	        W.push_back(e);
	    	e1.source = v;
	    	while(true){             
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
                nxt_s = true;
	            Configuration c1 = createConfiguration(nxt_m, v.query);
	            e1.targets.push_back(c1);          

	            } else break;
	        }
            if(nxt_s){

                W.push_back(e1);
            }
	    } else if (v.query->path == G){
	    	Configuration c = createConfiguration(v.marking, v.query->first);
	    	Edge e;
	    	e.source = v;
	    	e.targets.push_back(c);
	    	while(true){             
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){ 
	            Configuration c1 = createConfiguration(nxt_m, v.query); 
	            e.targets.push_back(c1);   
	            } else break;
	        }
	           W.push_back(e);
	    }
    } else if (v.query->quantifier == E){
    	if(v.query->path == U){
    		Configuration c = createConfiguration(v.marking, v.query->first);
    		Configuration c1 = createConfiguration(v.marking, v.query->second);
    		Edge e;
    		e.source = v;
    		e.targets.push_back(c1);
    	
    		while(true){ 
    		int i = 0;
    		Edge e1;
    		e1.source = v;
    		e1.targets.push_back(c);        
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
	            i++;
	            Configuration c1 = createConfiguration(nxt_m, v.query);
	            e1.targets.push_back(c1);          
	        	} else break;
	        if(i > 0){ W.push_back(e1); }
	        }
            W.push_back(e);
	  	} else if(v.query->path == X){
	  		while(true){   
            bool nxt_s = false;  
    		Edge e1;
    		e1.source = v;       
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
                nxt_s = true;
	            Configuration c1 = createConfiguration(nxt_m, v.query->first);
	            e1.targets.push_back(c1);          
	        	} else break;
            if(nxt_s){  W.push_back(e1); }     
            }
	   	} else if(v.query->path == F){
	  		Configuration c = createConfiguration(v.marking, v.query->first);
	  		Edge e;
	  		e.source = v;
	  		e.targets.push_back(c);
	  		
	  		while(true){     
            bool nxt_s = false;
    		Edge e1;
    		e1.source = v;       
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
                nxt_s = true;
	            Configuration c1 = createConfiguration(nxt_m, v.query);
	            e1.targets.push_back(c1);          
	        	} else break;
            if(nxt_s){ W.push_back(e1); }

	        }
            W.push_back(e);
	  	} else if(v.query->path == G){
            int i = 0;
	  		while(true){
	  		Configuration c = createConfiguration(v.marking, v.query->first);
	  		Edge e;
	  		e.source = v;
	  		e.targets.push_back(c);           
	        PetriEngine::MarkVal* nxt_m = new PetriEngine::MarkVal[_nplaces];
	            if(next_state(v.marking, nxt_m) == 1){
	            Configuration c1 = createConfiguration(nxt_m, v.query);
	            e.targets.push_back(c1);          
	        	} else { if (i == 0)  W.push_back(e); break;  }
            i++;
            W.push_back(e);
	        }
	  	}
    } else if (v.query->quantifier == AND){
    	Configuration c = createConfiguration(v.marking, v.query->first);
    	Configuration c1 = createConfiguration(v.marking, v.query->second);
    	Edge e;
    	e.source = v;
    	e.targets.push_back(c);
    	e.targets.push_back(c1);
    	W.push_back(e);
    } else if (v.query->quantifier == OR){
    	Configuration c = createConfiguration(v.marking, v.query->first);
    	Configuration c1 = createConfiguration(v.marking, v.query->second);
    	Edge e;
    	Edge e1;
    	e.source = v;
    	e1.source = v;
    	e.targets.push_back(c);
    	e1.targets.push_back(c1);
    	W.push_back(e);
        W.push_back(e1);
    } else if (v.query->quantifier == NEG){
    		Configuration c = createConfiguration(v.marking, v.query->first);
    		Edge e;
    		e.source = v;
    		if(!(localSmolka(c))){
    		*(v.assignment) = ONE;
            W.insert(W.end(), v.denpendencyList.begin(), v.denpendencyList.end());

    		} else{		
    		e.targets.push_back(c);
    		W.push_back(e);
    		}

    		//Make stuff that goes here
    } else {
    	if (evaluateQuery(v.marking, v.query)){
            Edge e;
            e.source = v;
            assignConfiguration(v, ONE);
            Configuration &c = v;
            e.targets.push_back(c);
            W.push_back(e);

		} else {
            Edge e;
            e.source = v;
            assignConfiguration(v, CZERO);
            Configuration &c = v;
            e.targets.push_back(c);
            W.push_back(e);
      }
    } 
}

bool CTLEngine::evaluateQuery(PetriEngine::MarkVal *marking, CTLTree *query){
    if (query->a.isFireable) {
        bool canFire = false;
        std::vector<int> possibleTransitionById = calculateFireableTransistions(marking);
        
        for (int i = 0; i < possibleTransitionById.size(); i++) {
        int j = 0;
			while(query->a.fireset[j] != NULL){
				if (strcmp(_net->transitionNames()[possibleTransitionById[i]].c_str(), query->a.fireset[j]) == 0){
	        		return true;           
	        	}
	        	j++;
	        }
        }
        return canFire;
    }
    else if (!(query->a.tokenCount.intSmaller == -1)) {
        bool isLessThan = false;
        int firstIntConst = query->a.tokenCount.intSmaller;
        //cout << "eval: " << firstIntConst << " <= " << flush;
        if (firstIntConst <= secondEvaluatedParam(marking, query))
            isLessThan = true;
        return isLessThan;
    }
    else if (!(query->a.tokenCount.placeSmaller == NULL)) {
        bool isLessThan = false;
        int firstPlaceCount;
        for (int i = 0; i < _nplaces; i++) {
            if (0 == (strcmp(query->a.tokenCount.placeSmaller, _net->placeNames()[i].c_str())))
                    firstPlaceCount = int(marking[i]); 
        }
        if (firstPlaceCount <= secondEvaluatedParam(marking, query))
            isLessThan = true;
        return isLessThan;
    }
    else
        return false;
}

int CTLEngine::secondEvaluatedParam(PetriEngine::MarkVal *marking, CTLTree *query) {
    int secondParam;
    if (!(query->a.tokenCount.intLarger == -1))
        secondParam = query->a.tokenCount.intLarger;
    else {
        for (int i = 0; i < _nplaces; i++) {
            if (0 == (strcmp(query->a.tokenCount.placeLarger, _net->placeNames()[i].c_str()))){
            		//cout << "place " << query->a.tokenCount.placeLarger << " " << flush;
                    secondParam = int(marking[i]); 
            }
        }
    }

    //cout << secondParam << "\n" << flush;
    return secondParam;
}

int CTLEngine::next_state(PetriEngine::MarkVal* current_m, PetriEngine::MarkVal* next_m){

    bool found = false;
    int nr_t = 0;


    CTLEngine::mIter m;

    if(!list.empty()){
        for (m = list.begin(); m != list.end(); m++) 
        {
            if(compareMarking(m->marking, current_m)){
            	if(!m->possibleTransitions.empty()){
	                found = true;
	                nr_t = m->index;
	                if (nr_t < m->possibleTransitions.size()){
	                    m->index++;
	                    makeNewMarking(current_m, m->possibleTransitions[nr_t], next_m);
	                    return 1;
	                }
	                else { m->index = 0; return 2;}
	            }
            else return 2;
           	}
        }
    }



    if (found == false){
        std::vector<int> tempPossibleTransitions = calculateFireableTransistions(current_m);
     	if(!tempPossibleTransitions.empty()){
	        CTLEngine::Markings temp;
	        temp.possibleTransitions = tempPossibleTransitions;
	        temp.index = 1;
	        temp.marking = current_m;
	        list.push_back(temp);
	        makeNewMarking(current_m, tempPossibleTransitions[0], next_m);
			return 1;
		} else {
			CTLEngine::Markings temp;
	        temp.possibleTransitions = tempPossibleTransitions;
	        temp.index = 1;
	        temp.marking = current_m;
	        list.push_back(temp);
	        return 2;
		}
    }

}

CTLEngine::Configuration CTLEngine::createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query){
    bool shouldBeNegated;
    Assignment* a = (Assignment*)malloc(sizeof(Assignment));

    if(query->quantifier == NEG){
        shouldBeNegated = true;
    }
    else {
        shouldBeNegated = false;
    }

    *a = UNKNOWN;

    Configuration newConfig = {
        marking, //marking
        query, //query
        a, //assignment
        CTLEngine::_nplaces, //mCount
        shouldBeNegated
    };

    return *((uset.insert(newConfig)).first);
}
/*
// This version of createConfiguration makes use of a vector to hold all configurations
// Function makes use of the == operator on a Configuration
CTLEngine::Configuration CTLEngine::createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query){
    
    bool shouldBeNegated;
    Assignment* a = (Assignment*)malloc(sizeof(Assignment));

    if(query->quantifier == NEG){
        shouldBeNegated = true;
    }
    else {
        shouldBeNegated = false;
    }

    *a = UNKNOWN;

    Configuration newConfig = {
        marking, //marking
        query, //query
        a, //assignment
        CTLEngine::_nplaces, //mCount
        shouldBeNegated
    };

    auto iterator = configlist.begin();
    
    while(iterator != configlist.end()){
        
        if( (*iterator) == newConfig){
            //Element already exists, no need to look any further
     
            return *iterator;
        }
        iterator++;
    }    
    
    configlist.push_back(newConfig);
    //Last element is the newly inserted Configuration

    return configlist.back();
}
*/
void CTLEngine::pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]){
    std::cout << "--------------- Petri Net Information -------------------\n";
    
    std::cout << "******** Number of Places: " << net->numberOfPlaces() << " \n";
    std::cout << "******** Number of Transitions: " << net->numberOfTransitions() << " \n";
    std::cout << "******** Place names: \n";
    for(std::vector<int>::size_type i = 0; i != net->placeNames().size(); i++){
        std::cout << "-------------- " << net->placeNames()[i] << "\n";
    }
    std::cout << "******** Transition names: \n";
    for(std::vector<int>::size_type i = 0; i != net->transitionNames().size(); i++){
        std::cout << "-------------- " << net->transitionNames()[i] << "\n";
    }
    
    int j;
    std::cout << "******** Initial Marking: \n";
    for(j = 0; j < net->numberOfPlaces(); j++){
        std::cout << "-------------- PlaceID " << j << ": " << initialmarking[j] << "\n";
    }
    
    std::cout << "---------------------------------------------------------\n";
}

void CTLEngine::configPrinter(CTLEngine::Configuration c){
    std::cout << "--------------- Configuration Information -------------------\n";
    CTLParser ctlParser = CTLParser();
    int i = 0;
    cout << "Configuration marking: " << flush;
    for (i = 0; i < _net->numberOfPlaces(); i++) {
        std::cout << c.marking[i]<< flush;
    }
    std::cout << "\nConfiguration query::::\n" ;
    ctlParser.printQuery(c.query);
    std::cout << "\nConfiguration assignment: " << *(c.assignment)<<"\n";

    std::cout << "\nshould be negated?: " << c.shouldBeNegated <<"\n";

    
    std::cout << "---------------------------------------------------------\n";
}

void CTLEngine::edgePrinter(CTLEngine::Edge e){

    std::cout << "--------------- Edge Information -------------------\n";
    std::cout << "--------------- source config----------------------\n";
    configPrinter(e.source);
    std::cout << "--------------- target configs----------------------\n";
    for(int i = 0; i < e.targets.size(); i++)
         configPrinter(e.targets.at(i));
    

    std::cout << "---------------------------------------------------------\n";

}

bool CTLEngine::compareMarking(PetriEngine::MarkVal m[], PetriEngine::MarkVal m1[]){
        for(int i = 0; i < _nplaces; i++)
        {
            if (m[i] != m1[i])
                return false;
        }
        return true;
}

void CTLEngine::makeNewMarking(PetriEngine::MarkVal m[], int t, PetriEngine::MarkVal nm[]){
    memcpy(nm, m, (sizeof(int)*_nplaces));
    for(int p = 0; p < _nplaces; p++){
        int place = nm[p] - _net->inArc(p,t);  
        nm[p] = place + _net->outArc(t,p);
    }
}

std::vector<int> CTLEngine::calculateFireableTransistions(PetriEngine::MarkVal m[]){
    std::vector<int> pt;

    for(int t = 0; t < _ntransitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < _nplaces; p++){
            if(m[p] < _net->inArc(p,t)) 
                transitionFound = false;
        }

        if(transitionFound)
            pt.push_back(t); 
    }
    return pt;
}  //possibleTransitions


bool CTLEngine::calculateCZERO(CTLEngine::Edge e, std::vector<CTLEngine::Edge>& W){

    bool propegateCZERO = true;

    std::vector<CTLEngine::Edge> czeroEdges;
    czeroEdges.push_back(e);

    for(int i = 0; i < W.size(); i++){
        if(W[i].source == e.source){
           czeroEdges.push_back(W[i]);
        }
    }

    /*cout << "WE ARE NOW IN THE CALC THING NOW WHAT ARE THE EDGES WE ARE COMPARING WITH" << flush;
    for(int i = 0; i < czeroEdges.size(); i++){
        edgePrinter(czeroEdges.at(i));
    } */

    auto zeroIt = czeroEdges.begin();

    if(zeroIt == czeroEdges.end()) {
        propegateCZERO = false;
        for(int c = 0; c < e.targets.size(); c++){
            if(*(e.targets.at(c).assignment) == CZERO)
                propegateCZERO = true;
        }
    }


    
    else{
        while(zeroIt != czeroEdges.end()){

            bool isEdgeGood = false;

            for(int i = 0; i < (*zeroIt).targets.size(); i++){
                if(*((*zeroIt).targets.at(i).assignment) == CZERO){
                    isEdgeGood = true;
                }
                if(isEdgeGood) break;
            }

            if(!isEdgeGood) {propegateCZERO = false; return propegateCZERO;}
            else zeroIt++;
        }
    }

    return propegateCZERO;    
}

void CTLEngine::RunEgineTest(){
    //Output section
    cout<<"::::::::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<":::::::::::::::::Engine Test::::::::::::::::::"<<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<":::::::NET INFORMATION"<<endl;
    cout<<"::::::::: Places:"<<endl;
    cout<<"::::::::::::: 1. "<<_net->placeNames()[0] <<endl;
    cout<<"::::::::::::: 2. "<<_net->placeNames()[1] <<endl;
    cout<<"::::::::::::: 3. "<<_net->placeNames()[2] <<endl;
    cout<<"::::::::::::: 4. "<<_net->placeNames()[3] <<endl;
    cout<<"::::::::: Transitions:"<<endl;
    cout<<"::::::::::::: 1. "<<_net->transitionNames()[0] <<endl;
    cout<<"::::::::::::: 2. "<<_net->transitionNames()[1] <<endl;
    cout<<"::::::::::::: 3. "<<_net->transitionNames()[2] <<endl;
    cout<<"::::::::: Arcs:"<<endl;
    cout<<"::::::::::::: From "<<_net->transitionNames()[0]<<" to "<< _net->placeNames()[0] << " Weight: "<<_net->outArc(0,0) <<endl;
    cout<<"::::::::::::: From "<<_net->transitionNames()[0]<<" to "<< _net->placeNames()[1] << " Weight: "<<_net->outArc(0,1) <<endl;
    cout<<"::::::::::::: From "<<_net->transitionNames()[1]<<" to "<< _net->placeNames()[2] << " Weight: "<<_net->outArc(1,2) <<endl;
    cout<<"::::::::::::: From "<<_net->transitionNames()[1]<<" to "<< _net->placeNames()[3] << " Weight: "<<_net->outArc(1,3) <<endl;
    cout<<"::::::::::::: From "<<_net->transitionNames()[2]<<" to "<< _net->placeNames()[1] << " Weight: "<<_net->outArc(2,1) <<endl;
    cout<<"::::::::::::: From "<<_net->placeNames()[0]<<" to "<< _net->transitionNames()[1] << " Weight: "<<_net->inArc(0,1) <<endl;
    cout<<"::::::::::::: From "<<_net->placeNames()[1]<<" to "<< _net->transitionNames()[1] << " Weight: "<<_net->inArc(1,1) <<endl;
    cout<<"::::::::::::: From "<<_net->placeNames()[2]<<" to "<< _net->transitionNames()[0] << " Weight: "<<_net->inArc(2,0) <<endl;
    cout<<"::::::::::::: From "<<_net->placeNames()[3]<<" to "<< _net->transitionNames()[2] << " Weight: "<<_net->inArc(3,2) <<endl;
    cout<<"::::::::: Initial Marking:"<<endl;
    cout<<"::::::::::::: P0: "<<_m0[0] <<endl;
    cout<<"::::::::::::: P1: "<<_m0[1] <<endl;
    cout<<"::::::::::::: P2: "<<_m0[2] <<endl;
    cout<<"::::::::::::: P3: "<<_m0[3] <<endl;
    cout<<"::::::::::::::::::::::::::::::::::::::::::::::"<<endl;
    cout<<":::::::::::::::Running Test Setup:::::::::::::"<<endl;
    
    cout<<":::::::|";
    //Test 1 - readSatisfactory
    querySatisfied = true;
    assert(readSatisfactory() == true);
    cout<<"====";
    
    //Test 2 - localSmolka
    cout<<"====";
    
    //Test 3 - create configuration
    /** createConfiguration 
     ** - IN *marking, *query 
     ** - OUT configuration  **/
    cout<<"====";
    
    //Test 4 - successors
    cout<<"====";
    
    //Test 5 - evaluate atom
    cout<<"====";
    
    //Test 6 - calculate CZERO
    cout<<"====";
    
    //Test 7 - create marking
    PetriEngine::MarkVal* testMarking = new PetriEngine::MarkVal[_nplaces];
    makeNewMarking(_m0, 0, testMarking);
    assert(testMarking[0] == 1);
    assert(testMarking[1] == 1);
    assert(testMarking[2] == 1);
    assert(testMarking[3] == 0);
    cout<<"====";
    
    //Test 8 - compare two markings
    assert(compareMarking(_m0, testMarking) == false);
    assert(compareMarking(_m0, _m0) == true);
    cout<<"====";
    
    //Test 9 - calculate fireability
    vector<int> resvector = calculateFireableTransistions(_m0);
    assert(resvector[0] == 0);
    assert(resvector.size() == 1);
    cout<<"===";
    
    //Test 10 - Next state
    cout<<"==|"<<endl;
    
    cout<<"::::::::::::::::Test SUCCESSFUL:::::::::::::::"<<endl;
}