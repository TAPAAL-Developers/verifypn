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

#include "CTLEngine.h"


CTLEngine::CTLEngine() {
    
}

CTLEngine::CTLEngine(const CTLEngine& orig) {
}

CTLEngine::~CTLEngine() {
}

void CTLEngine::search(PetriEngine::PetriNet* net, PetriEngine::MarkVal* marking, string query_str){
    
    //CTLEngine.querySatisfied = true;
}
bool CTLEngine::readSatisfactory() {
    return false;
}