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
#include <map>

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



CTLEngine::CTLEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]) {
    _net = net;
    _m0 = initialmarking;
    _nplaces = net->numberOfPlaces();
    _ntransitions = net->numberOfTransitions();
}

CTLEngine::CTLEngine(const CTLEngine& orig) {
}

CTLEngine::~CTLEngine() {
}


//Public functions
void CTLEngine::search(CTLTree *query){
    pNetPrinter(_net, _m0);
    Configuration v0 = createConfiguration(_m0, query);
    querySatisfied = localSmolka(v0);
    
}
bool CTLEngine::readSatisfactory() {
    return querySatisfied;
}






//Private functions

bool CTLEngine::localSmolka(Configuration v){
    assignConfiguration(v, ZERO);
    std::vector<CTLEngine::Edge> D;
    std::vector<CTLEngine::Edge> W;
    successors(v, W);
    while (W.size() != 0) {
        int i = 0;
        Edge e = W.front();
        //MISSING: remove/pop e from W
        
        /*****************************************************************/
        /*Data handling*/
        int targetONEassignments = 0;
        int targetZEROassignments = 0;
        for (i = 0; i < (sizeof(e.targets) / sizeof(Configuration)); i++ ){
            if (e.targets[i].assignment == ONE) {
                targetONEassignments++;
            }
            else if (e.targets[i].assignment == ZERO) {
                targetZEROassignments++;
            }
        }
        /*****************************************************************/
        /*if A(u) = 1, ∀u ∈ T then A(v) ← 1; W ← W ∪ D(v);*/
        if (targetONEassignments == (sizeof(e.targets) / sizeof(Configuration))) {
            int j = 0;
            v.assignment = ONE;
            for (j = 0; j < D.size(); j++) {
                W.push_back(D.front());
                //remove/pop D.front from D
            }
        }
        /*****************************************************************/ 
        /*else if ∃u ∈ T where A(u) = 0 then D(u) ← D(u) ∪ {e}*/
        else if (targetZEROassignments > 0) {
            D.push_back(e);
        }
        /*****************************************************************/ 
        /*else if ∃u ∈ T where A(u) = ⊥ then A(u) ← 0; D(u) ← D(u) ∪ e; W ← W ∪ succ(u)*/
        else {
            for (i = 0; i < (sizeof(e.targets) / sizeof(Configuration)); i++ ){
                if (e.targets[i].assignment == UNKNOWN) {
                    Configuration u = e.targets[i];
                    assignConfiguration(u, ZERO);
                    D.push_back(e);
                    successors(u,W);
                }
            }
        }
        /*****************************************************************/ 
    }
    return v.assignment;
}

void CTLEngine::assignConfiguration(Configuration v, Assignment a) {
    v.assignment = a;
}



void CTLEngine::successors(Configuration v, std::vector<CTLEngine::Edge> W) {
    Configuration **succ;
    
    
    
    //v.successors = succ;
}

int CTLEngine::next_state(PetriEngine::MarkVal* current_m, PetriEngine::MarkVal* next_m){

    bool found = false;
    int nr_t = 0;


    CTLEngine::mIter m;

    if(!list.empty()){
        for (m = list.begin(); m != list.end(); m++) 
        {
            if(compareMarking(m->marking, current_m)){
                found = true;
                nr_t = m->index;
                if (nr_t < m->possibleTransitions.size()){
                    m->index++;
                    makeNewMarking(current_m, m->possibleTransitions[nr_t], next_m);
                    return 1;
                }
                else { m->index = 0; return 2;}
                }
            }
        }
    


    if (found == false){
        std::vector<int> tempPossibleTransitions = calculateFireableTransistions(current_m);
        CTLEngine::Markings temp;
        temp.possibleTransitions = tempPossibleTransitions;
        temp.index = 1;
        temp.marking = current_m;
        list.push_back(temp);
        makeNewMarking(current_m, tempPossibleTransitions[0], next_m);
        return 1;
    }

}

CTLEngine::Configuration CTLEngine::createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query){
    Configuration newConfig;
    newConfig.marking = marking;
    newConfig.query = query;
    newConfig.assignment = UNKNOWN;
    return newConfig;
}


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

bool CTLEngine::compareMarking(PetriEngine::MarkVal m[], PetriEngine::MarkVal m1[]){
        for(int i = 0; i < 4; i++)
        {
            if (m[i] != m1[i])
                return false;
        }
        return true;
}


void CTLEngine::makeNewMarking(PetriEngine::MarkVal m[], int t, PetriEngine::MarkVal nm[]){


        for(int p = 0; p < _nplaces; p++){
            nm[p] = m[p];
            int place = nm[p] - _net->inArc(p,t);  
            nm[p] = place + _net->outArc(t,p);
        }
       
}



std::vector<int> CTLEngine::calculateFireableTransistions(PetriEngine::MarkVal m[]){
    std::vector<int> pt;

    for(int t = 0; t < _ntransitions; t++){
        bool transitionFound = true;
        for(int p = 0; p < _nplaces; p++){
            if(m[p] < _net->inArc(p,t)) {transitionFound = false;}
        }

        if(transitionFound){ pt.push_back(t); }
    }
    return pt;
}  //possibleTransitions

