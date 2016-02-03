/* 
 * File:   CTLParser.cpp
 * Author: mossns
 * 
 * Created on September 30, 2015, 10:39 AM
 */

#include <string>
#include <fstream>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include "rapidxml-1.13/rapidxml.hpp"
 
#include "CTLParser.h"


using namespace rapidxml;

CTLParser::CTLParser() {
}

CTLParser::CTLParser(PetriEngine::PetriNet* net) {
    _net = net;
}

CTLParser::CTLParser(const CTLParser& orig) {
}

CTLParser::~CTLParser() {
}

void CTLParser::ParseXMLQuery(std::vector<char> buffer, CTLFormula *queryList[]) {
    #ifdef DEBUG
    std::cout << "Creating doc\n" << std::flush;
    #endif
//    CTLquery ctlquery;
    xml_document<> doc;
    xml_node<> * root_node;
    
    #ifdef DEBUG
    std::cout << "Size of Path enum: " << sizeof(Path)*8 <<"\n";
    #endif
    doc.parse<0>(&buffer[0]);
    

#ifdef DEBUG
    std::cout << "First node id: " << doc.first_node()->name() << std::endl;
#endif

    root_node = doc.first_node();

#ifdef Analysis
    std::cout << "\nAnalysis:: Queries:" << std::endl;
#endif

    int i = 0;
    for (xml_node<> * property_node = root_node->first_node("property"); property_node; property_node = property_node->next_sibling()) {
        xml_node<> * id_node = property_node->first_node("id"); 
        queryList[i] = (CTLFormula*)malloc(sizeof(CTLFormula));

        //Set competition information
        int size = id_node->value_size();
        queryList[i]->Name = strcpy((char*)calloc(size, sizeof(char)*size),
                                    id_node->value());
        queryList[i]->Result = false;
        queryList[i]->Techniques = new std::vector<std::string>();

        //Fill list with individual queries
        xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
        queryList[i]->Query = xmlToCTLquery(formula_node->first_node());

        printQuery(queryList[i]->Query);
        std::cout << "\n";

        i++;
    }
}

CTLTree* CTLParser::xmlToCTLquery(xml_node<> * root) {
    bool isA = false;
    bool isE = false;
    CTLTree *query = (CTLTree*)malloc(sizeof(CTLTree));
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    //Parse query segment
    if (firstLetter == 'a') {
        //Construct All (Transformed if AG)
        isA = true;
        query->quantifier = A;
        query->path = setPathOperator(root->first_node(), isA, isE);
        isA = false;
        if(isAG){
            createAGquery(root, query);
            return query;
        }
        
    }
    else if (firstLetter == 'e' ) {
        //Construct Exist (Transformed if EG)
    	isE = true;
        query->quantifier = E;
        query->path = setPathOperator(root->first_node(), isA, isE);
        isE = false;
        if(isEG){
            createEGquery(root, query);
            return query;
        }
        
    }
    else if (firstLetter == 'n' ) {
        //Construct negation
        query->quantifier = NEG;
        
    }
    else if (firstLetter == 'c' ) {
        //Construct conjunction
        query->quantifier = AND;
    }
    else if (firstLetter == 'd' ) {
        //Construct disjunction
        query->quantifier = OR;
    }
    else if (firstLetter == 'i' ) {
        //Construct Atom
        query->quantifier = EMPTY;
        query->depth = 0;
        query->max_depth = 0;
        if (root_name[1] == 's' ) { // Check if Fireability query
            constructFireAtom(query, root);
            return query; 
        }
        else if (root_name[1] == 'n') { // Check if Cardinality query
            constructCardinalityAtom(query, root);
            return query; 
        }
        else {
            #ifdef DEBUG
            std::cout << "ERROR in xmlToCTLquery: Invalid atom " << root_name << "\n";
            #endif
        }
    }
    else {
        #ifdef DEBUG
        std::cout << "ERROR in xmlToCTLquery: Invalid boolean operator: " << root_name << "\n";
        #endif
    }
    
    
    //Create query children
    if (query->path == pError) {
        #ifdef DEBUG
        std::cout << "ERROR in xmlToCTLquery: !!Exiting - parse error!!\n";
        #endif
    }
    else if (query->path == U) {
        xml_node<> * child_node = root->first_node()->first_node();
        query->first = xmlToCTLquery(child_node->first_node());
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node->first_node());
    }
    else if (query->quantifier == AND || query->quantifier == OR) {
        xml_node<> * child_node = root->first_node();
        query->first = xmlToCTLquery(child_node);
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node);
    }
    else if (query->quantifier == NEG) {
        query->first = xmlToCTLquery(root->first_node());
    }
    else {
        query->first = xmlToCTLquery(root->first_node()->first_node());
    }
    
    
    //Update query depth information
    if (query->quantifier == AND || query->quantifier == OR || query->path == U){
        int first_depth = query->first->depth, second_depth = query->second->depth;
        query->depth = (lowerDepth(first_depth, second_depth) + 1);
        
        int first_max_depth = query->first->max_depth, second_max_depth = query->second->max_depth;
        query->max_depth = (higherDepth(first_max_depth, second_max_depth) + 1);
    }
    else {
        query->depth = (query->first->depth + 1);
        query->max_depth = (query->first->max_depth + 1);
    }
    
    return query;
}

Path CTLParser::setPathOperator(xml_node<> * root, bool isA, bool isE) {
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    if (firstLetter == 'g') {
        if(isA) { isAG = true; }
        if(isE){ isEG = true; }
        return G;
    }
    else if (firstLetter == 'f') {
        isEG = false;
        isAG = false;
        return F;
    }
    else if (firstLetter == 'n') {
        isEG = false;
        isAG = false;
        return X;
    }
    else if (firstLetter == 'u') {
        isEG = false;
        isAG = false;
        return U;
    }
    else {
        #ifdef DEBUG
        std::cout << "ERROR in setPathOperator: Invalid path operator: " << root_name << "\n";
        #endif
    }
    return pError;
}

void CTLParser::printQuery(CTLTree *query) {
    if(query->quantifier == EMPTY) {
        Atom a = query->a;
        if(a.isFireable){
            std::cout<< " IsFireable(" << std::flush;
            int i = 0;
            for(i = 0; i < query->a.firesize; i++){
                int j = 0;
                for (j = 0; j < query->a.fireset[i].sizeofdenpencyplaces; j++){
                    std::cout<<"t_name_not_available" << std::flush;
                    if(j < (query->a.fireset[i].sizeofdenpencyplaces - 1))
                        std::cout << ", ";
                }
            }
            std::cout << ")";
            return;
        }
        else {
            std::cout<< " Tokencount(" << std::flush;
            if (a.cardinality.intSmaller == -1){
                int i = 0;
                for (i = 0; i < a.cardinality.placeSmaller.sizeoftokencount; i++){
                    std::cout<< a.cardinality.placeSmaller.cardinality[i]<<" ";
                }
            }
            else 
                std::cout << a.cardinality.intSmaller;
            
            std::cout<< " le " << std::flush;
            
            if (a.cardinality.intLarger == -1) {
                int i = 0;
                for (i = 0; i < a.cardinality.placeLarger.sizeoftokencount; i++){
                    std::cout<< a.cardinality.placeLarger.cardinality[i]<<" ";
                }
            }
            else 
                std::cout<< a.cardinality.intLarger;
            std::cout << ")";
        }
        return;
    }
    else if (query->quantifier == NEG){
        std::cout << "!("; printQuery(query->first) ;std::cout <<")" << std::flush;
    }
    else if (query->quantifier == AND){
        std::cout << "(" ; printQuery(query->first);std::cout <<") AND ("; printQuery(query->second);std::cout<< ")" << std::flush;
    }
    else if (query->quantifier == OR){
        std::cout << "("; printQuery(query->first);std::cout <<") OR ("; printQuery(query->second);std::cout << ")" << std::flush;
    }
    else if (query->quantifier == A){
        std::cout << "A" << std::flush;
        printPath(query);
    }
    else if ( query->quantifier == E){
        std::cout << "E" << std::flush;
        printPath(query);
    }
    else return;
}

void CTLParser::printPath(CTLTree *query) {
    if (query->path == G)
        std::cout << "G";
    else if (query->path == F)
        std::cout << "F";
    else if (query->path == U)
        {std::cout << "("; printQuery(query->first);std::cout <<") U ("; printQuery(query->second);std::cout << ")"; return;}
    else if (query->path == X)
        std::cout << "X";
    printQuery(query->first);
}

bool CTLParser::charEmpty(char *query) {
    if (query == NULL) {
        return true;
    }
    return false;
}

void CTLParser::constructCardinalityAtom(CTLTree *query, xml_node<> *root){
    xml_node<> * integerNode = root->first_node();
    query->a.isFireable = false;
    query->a.firesize = 0;
    
    constructPreOperator(integerNode, query);
    
    integerNode = integerNode->next_sibling();
    
    constructPostOperator(integerNode, query);
}

void CTLParser::constructPreOperator(xml_node<> * integerNode, CTLTree *query){
    if (integerNode->name()[0] == 't') {
        int smallPlacecount = 0, numberofplaces = 0;
        for (xml_node<> * place_node = integerNode->first_node(); place_node; place_node = place_node->next_sibling()) {
            numberofplaces++;
        }
        //std::cout<<"Number of places: "<<numberofplaces<<std::flush;

        query->a.cardinality.placeSmaller.cardinality = (int*) calloc(numberofplaces, sizeof(int));
        query->a.cardinality.placeSmaller.sizeoftokencount = numberofplaces;

        for (xml_node<> * place_node = integerNode->first_node(); place_node; place_node = place_node->next_sibling()) {
            int p_index = 0;
            for (p_index = 0; p_index < _net->numberOfPlaces(); p_index++){
                if(strcmp(_net->placeNames()[p_index].c_str(),place_node->value()) == 0){
                    query->a.cardinality.placeSmaller.cardinality[smallPlacecount] = p_index;
                    smallPlacecount++;
                    break;
                }
            }
        }
        query->a.cardinality.intSmaller = -1;
       // std::cout<< query->a.tokenCount.placeSmaller << " should be a smaller PLACE than ";
    }
    else if (integerNode->name()[0] == 'i') {
        char *temp;
        temp = integerNode->value();
        query->a.cardinality.intSmaller = atoi(temp);
        query->a.cardinality.placeSmaller.sizeoftokencount = 0;         
        //std::cout<< query->a.cardinality.intSmaller << " should be a smaller INTEGER-CONTANT than ";
    }
}

void CTLParser::constructPostOperator(xml_node<> * integerNode, CTLTree *query){
    if (integerNode->name()[0] == 't') {
        int largePlacecount = 0, numberofplaces = 0;
        for (xml_node<> * place_node = integerNode->first_node(); place_node; place_node = place_node->next_sibling()) {
            numberofplaces++;
        }
        //std::cout<<"Number of places: "<<numberofplaces<<std::endl;
        query->a.cardinality.placeLarger.cardinality = (int*) calloc(numberofplaces, sizeof(int));
        query->a.cardinality.placeLarger.sizeoftokencount = numberofplaces;
        for (xml_node<> * place_node = integerNode->first_node(); place_node; place_node = place_node->next_sibling()) {
            int p_index = 0;
            for (p_index = 0; p_index < _net->numberOfPlaces(); p_index++){
                if(strcmp(_net->placeNames()[p_index].c_str(),place_node->first_node()->value()) == 0){
                    query->a.cardinality.placeLarger.cardinality[largePlacecount] = p_index;
                    largePlacecount++;
                    break;
                }
            }
        }
        query->a.cardinality.intLarger = -1;
    }

    else if (integerNode->name()[0] == 'i') {
        char *temp;
        temp = integerNode->value();
        query->a.cardinality.intLarger = atoi(temp);
        query->a.cardinality.placeLarger.sizeoftokencount = 0;
        //std::cout<< query->a.cardinality.intLarger << " - witch is an INTEGER-CONTANT ";
    }
}

void CTLParser::constructFireAtom(CTLTree *query, xml_node<> *root){
    numberoftransitions = 0;
    int i = 0, numberofdependencyplaces = 0;
    query->a.isFireable = true;
    //std::cout << "TEST:: Current set made" <<std::endl;
    for (xml_node<> * transition_node = root->first_node(); transition_node; transition_node = transition_node->next_sibling()) {
        numberoftransitions++;
    }
    //numberoftransitions++;
    //std::cout << "TEST:: Number of transitions: "<<numberoftransitions <<std::endl;
    query->a.firesize = numberoftransitions;
    query->a.fireset = (Fireability*) calloc(numberoftransitions, sizeof(Fireability));


    for (xml_node<> * transition_node = root->first_node(); transition_node; transition_node = transition_node->next_sibling()) {
        int t_count = 0;
        for (t_count = 0; t_count < _net->numberOfTransitions(); t_count++){
            if (strcmp( _net->transitionNames()[t_count].c_str(), transition_node->value()) == 0){
                numberofdependencyplaces = countNumberofDependencies(t_count);
                query->a.fireset[i].denpencyplaces = (Dependency*) calloc(numberofdependencyplaces, sizeof(Dependency));
                query->a.fireset[i].sizeofdenpencyplaces = numberofdependencyplaces;
            }
        }
        for (t_count = 0; t_count < _net->numberOfTransitions(); t_count++){
            if (strcmp( _net->transitionNames()[t_count].c_str(), transition_node->value()) == 0){
                convertToCardinality(i, query, t_count);
            }
        }

        i++;
    }
}

void CTLParser::convertToCardinality(int i, CTLTree *query, int t_index){
    int p_count = 0;
    int dependent_count = 0;
    for(p_count = 0; p_count < _net->numberOfPlaces(); p_count++){
        int arc_weight = _net->inArc(p_count, t_index);
        if (arc_weight > 0){
            query->a.fireset[i].denpencyplaces[dependent_count].intSmaller = arc_weight;
            query->a.fireset[i].denpencyplaces[dependent_count].placeLarger = p_count;
            dependent_count++;
        }
    }
}

int CTLParser::countNumberofDependencies(int t_index){
    //Returns how many places the transition is dependent on
    int p_count = 0, res = 0;
    for(p_count = 0; p_count < _net->numberOfPlaces(); p_count++){
        int arc_weight = _net->inArc(p_count, t_index);
        if (arc_weight > 0){
            res++;
        }
    }
    return res;
}

void CTLParser::createAGquery(xml_node<> * root, CTLTree *query){
    //Function for converting AG to !EF!
    CTLTree *query1 = (CTLTree*)malloc(sizeof(CTLTree));
    CTLTree *query2 = (CTLTree*)malloc(sizeof(CTLTree));

    //Set the first operator to neg
    query->quantifier = NEG;
    query->a.fireset = NULL;
    query->first = query1;

    //Set the second operator to EF
    query1->quantifier = E;
    query1->path = F;
    query1->a.fireset = NULL;
    query1->first = query2;

    ////Set the third operator to neg
    query2->quantifier = NEG;
    query2->a.fireset = NULL;
    
    //Recursively find successor query
    query2->first = xmlToCTLquery(root->first_node()->first_node());

    //Update depth information
    isAG = false;
    query2->max_depth = (query2->first->max_depth + 1);
    query2->depth = (query2->first->depth + 1);
    query1->depth = (query2->depth + 1);
    query1->max_depth = (query2->max_depth + 1);
    query->depth = (query1->depth + 1);
    query->max_depth = (query1->max_depth + 1);
}

void CTLParser::createEGquery(xml_node<> * root, CTLTree *query){
    //Function for converting EG to !AF!
    CTLTree *query1 = (CTLTree*)malloc(sizeof(CTLTree));
    CTLTree *query2 = (CTLTree*)malloc(sizeof(CTLTree));
    
    //Set the first operator to neg
    query->quantifier = NEG;
    query->a.fireset = NULL;
    query->first = query1;

    //Set the second operator to AF
    query1->quantifier = A;
    query1->path = F;
    query1->a.fireset = NULL;
    query1->first = query2;
    
    ////Set the third operator to neg
    query2->quantifier = NEG;
    query2->a.fireset = NULL;
    
    //Recursively find successor query
    query2->first = xmlToCTLquery(root->first_node()->first_node());
    
    //Update depth information
    isEG = false;
    query2->depth = (query2->first->depth + 1);
    query2->max_depth = (query2->first->max_depth + 1);
    query1->depth = (query2->depth + 1);
    query1->max_depth = (query2->max_depth + 1);
    query->depth = (query1->depth + 1);
    query->max_depth = (query1->max_depth + 1);
}

unsigned int CTLParser::lowerDepth(unsigned int a, unsigned int b){
    if (a < b)
        return a;
    else if (a == b)
        return a;
    else return b;
}

unsigned int CTLParser::higherDepth(unsigned int a, unsigned int b){
    if (b > a)
        return b;
    else if (a == b)
        return b;
    else return a;
}

//Test functions
void CTLParser::RunParserTest(){
    CTLFormula *queryList[12];
    std::string querypath = "testFramework/unitTestResources/TEST_CTLFireabilitySimple.xml";
            //"testFramework/unitTestResources/TEST_CTLFireabilitySimple.xml";
    const char* queryfile = querypath.c_str();
    std::ifstream xmlfile (queryfile);
    std::vector<char> buffer((std::istreambuf_iterator<char>(xmlfile)), std::istreambuf_iterator<char>());
    buffer.push_back('\0');
    
    
    //Test 1 - charEmpty
    char* test1input = NULL;
    bool resTest1 = charEmpty(test1input);
    assert(resTest1 == true);
  
    
    //Test 2 - Correct queries
    ParseXMLQuery(buffer, queryList);
        //Confirm AF becomes AF
        assert(queryList[0]->Query->quantifier == A);
        assert(queryList[0]->Query->path == F);
        //Confirm EF becomes EF
        assert(queryList[1]->Query->quantifier == E);
        assert(queryList[1]->Query->path == F);
        //Confirm AG becomes !EF!()
        assert(queryList[2]->Query->quantifier == NEG);
        assert(queryList[2]->Query->first->quantifier == E);
        assert(queryList[2]->Query->first->path == F);
        assert(queryList[2]->Query->first->first->quantifier == NEG);
        //Confirm EG becomes !AF!()
        assert(queryList[3]->Query->quantifier == NEG);
        assert(queryList[3]->Query->first->quantifier == A);
        assert(queryList[3]->Query->first->path == F);
        assert(queryList[3]->Query->first->first->quantifier == NEG);
        //Confirm AU becomes AU
        assert(queryList[4]->Query->quantifier == A);
        assert(queryList[4]->Query->path == U);
        //Confirm EU becomes EU
        assert(queryList[5]->Query->quantifier == E);
        assert(queryList[5]->Query->path == U);
        //Confirm AX becomes AX
        assert(queryList[6]->Query->quantifier == A);
        assert(queryList[6]->Query->path == X);
        //Confirm EX becomes EX
        assert(queryList[7]->Query->quantifier == E);
        assert(queryList[7]->Query->path == X);
        //Confirm Houseconstructions
        int i = 0;
        
        std::cout<<"::::::::::::::::::::::::::::::::\n:::::::: Parse Test End ::::::::\n::::::::::::::::::::::::::::::::\n"<<std::endl;
        
}
