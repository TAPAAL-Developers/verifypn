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

        int size = id_node->value_size();
        queryList[i]->Name = strcpy((char*)calloc(size, sizeof(char)*size),
                                  id_node->value());

        queryList[i]->Result = false;
        queryList[i]->Techniques = new std::vector<std::string>();
        
      
#ifdef Analysis
        std::cout << "\nAnalysis:: Query: " << id_node->value() << std::endl;
#endif
        xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
        queryList[i]->Query = xmlToCTLquery(formula_node->first_node());

        //#ifdef PP
        /*printQuery(queryList[i]->Query);
        std::cout << "\n";*/
        //#endif

        #ifdef DEBUG
        std::cout << "\n";
        #endif

        i++;
    }
}

CTLTree* CTLParser::xmlToCTLquery(xml_node<> * root) {
    bool isA = false;
    bool isE = false;
    CTLTree *query = (CTLTree*)malloc(sizeof(CTLTree));
    char *root_name = root->name();
    //std::cout << "TEST:: Running xmlToCTLquery with " << root_name << " as root\n";
    char firstLetter = root_name[0];
    
    if (firstLetter == 'a') {
        isA = true;
        query->quantifier = A;
        query->path = setPathOperator(root->first_node(), isA, isE);
        isA = false;
        if(isAG){
            CTLTree *query1 = (CTLTree*)malloc(sizeof(CTLTree));
            CTLTree *query2 = (CTLTree*)malloc(sizeof(CTLTree));

            query->quantifier = NEG;
            query->a.fireset = NULL;
            query->first = query1;

            query1->quantifier = E;
            query1->path = F;
            query1->a.fireset = NULL;
            query1->first = query2;

            query2->quantifier = NEG;
            query2->a.fireset = NULL;
            query2->first = xmlToCTLquery(root->first_node()->first_node());
            query2->depth = (query2->first->depth + 1);

            isAG = false;
            query1->depth = (query2->depth + 1);
            query->depth = (query1->depth + 1);
            
            return query;
        }
        
    }
    else if (firstLetter == 'e' ) {
    	isE = true;
        query->quantifier = E;
        query->path = setPathOperator(root->first_node(), isA, isE);
        isE = false;
        if(isEG){
            
            CTLTree *query1 = (CTLTree*)malloc(sizeof(CTLTree));
            CTLTree *query2 = (CTLTree*)malloc(sizeof(CTLTree));

            query->quantifier = NEG;
            query->a.fireset = NULL;
            query->first = query1;

            query1->quantifier = A;
            query1->path = F;
            query1->a.fireset = NULL;
            query1->first = query2;

            query2->quantifier = NEG;
            query2->a.fireset = NULL;
            query2->first = xmlToCTLquery(root->first_node()->first_node());
            query2->depth = (query2->first->depth + 1);
            isEG = false;
            //std::cout<<"Made subquery for EG:"<<std::endl;
            //printQuery(query);
            query1->depth = (query2->depth + 1);
            query->depth = (query1->depth + 1);
            
            return query;
        }
        
    }
    else if (firstLetter == 'n' ) {
        query->quantifier = NEG;
        //std::cout << "TEST:: Q: " << root_name << "\n";
        //query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'c' ) {
        query->quantifier = AND;
        //std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'd' ) {
        query->quantifier = OR;
        //std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'i' ) {
        query->quantifier = EMPTY;
        query->depth = 0;
        //std::cout << "TEST:: ATOM: " << root_name << "\n";
        if (root_name[1] == 's' ) {
            numberoftransitions = 0;
            int t_id= 0, i = 0, numberofdependencyplaces = 0;
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
                        int p_count = 0;
                        for(p_count = 0; p_count < _net->numberOfPlaces(); p_count++){
                            int arc_weight = _net->inArc(p_count, t_count);
                            if (arc_weight > 0){
                                numberofdependencyplaces++;
                            }
                        }
                        
                        query->a.fireset[i].denpencyplaces = (Cardinality*) calloc(numberofdependencyplaces, sizeof(Cardinality));
                        query->a.fireset[i].sizeofdenpencyplaces = numberofdependencyplaces;
                        
                        
                    }
                }
                for (t_count = 0; t_count < _net->numberOfTransitions(); t_count++){
                    if (strcmp( _net->transitionNames()[t_count].c_str(), transition_node->value()) == 0){
                        int p_count = 0;
                        int dependent_count = 0;
                        for(p_count = 0; p_count < _net->numberOfPlaces(); p_count++){
                            int arc_weight = _net->inArc(p_count, t_count);
                            if (arc_weight > 0){
                                query->a.fireset[i].denpencyplaces[dependent_count].intSmaller = arc_weight;
                                query->a.fireset[i].denpencyplaces[dependent_count].placeLarger = p_count;
                                dependent_count++;
                            }
                        }
                        if (dependent_count > numberofdependencyplaces){
                            std::cout << "-----TEST:: WE HAVE A PROBLEM!!\n" << std::flush;
                        }
                    }
                }
                
                i++;
            }
            
            //std::cout << "-----TEST:: Returning query atom from fireability: \n" << std::flush;
            //printQuery(query);
            //std::cout << "\n" << std::flush;
            
            return query; 
        }
        else if (root_name[1] == 'n') {
            xml_node<> * integerNode = root->first_node();
            integerNode = root->first_node();
            query->a.isFireable = false;
            query->a.firesize = 0;
            xml_node<> * temp_node = integerNode;
            //std::cout<< "\n\n ---------------> Found integer-le - First attribute:\n ::Name: "<<integerNode->name()<<"\n ::Value: "<<integerNode->value()<<"\n"<<std::flush;
            if (integerNode->name()[0] == 't') {
                int p_index = 0;
                for (p_index = 0; p_index < _net->numberOfPlaces(); p_index++){
                    if(strcmp(_net->placeNames()[p_index].c_str(),integerNode->first_node()->value()) == 0){
                        query->a.tokenCount.placeSmaller = p_index;
                    }
                }
                query->a.tokenCount.intSmaller = -1;
               // std::cout<< query->a.tokenCount.placeSmaller << " should be a smaller PLACE than ";
            }
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                query->a.tokenCount.intSmaller = atoi(temp);
                query->a.tokenCount.placeSmaller = -1;
              //  std::cout<< query->a.tokenCount.intSmaller << " should be a smaller INTEGER-CONTANT than ";
            }
            
            integerNode = integerNode->next_sibling();
            
            if (integerNode->name()[0] == 't') {
                int p_index = 0;
                for (p_index = 0; p_index < _net->numberOfPlaces(); p_index++){
                    if(strcmp(_net->placeNames()[p_index].c_str(),integerNode->first_node()->value()) == 0){
                        query->a.tokenCount.placeLarger = p_index;
                    }
                }
                query->a.tokenCount.intLarger = -1;
              //  std::cout<< query->a.tokenCount.placeLarger << " - witch is a PLACE ";
            }
                
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                query->a.tokenCount.intLarger = atoi(temp);
                query->a.tokenCount.placeLarger = -1;
               // std::cout<< query->a.tokenCount.intLarger << " - witch is an INTEGER-CONTANT ";
            }
            
            //std::cout << "-----TEST:: Returning query set: " << query->a.set << "\n" << std::flush;
            
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
    //std::cout << "TEST:: Returning " << root_name << "\n";
    
    if (query->quantifier == AND || query->quantifier == OR || query->path == U){
        int first_depth = query->first->depth, second_depth = query->second->depth;
        query->depth = (lowerDepth(first_depth, second_depth) + 1);
    }
    else query->depth = (query->first->depth + 1);
    
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
            std::cout<< " IsFireable(";
            int i = 0;
            for(i = 0; i < query->a.firesize; i++){
                int j = 0;
                for (j = 0; j < query->a.fireset[i].sizeofdenpencyplaces; j++){
                    std::cout<<"t_name_not_available";
                    if(j < (query->a.fireset[i].sizeofdenpencyplaces - 1))
                        std::cout << ", ";
                }
            }
            std::cout << ")";
            return;
        }
        else {
            std::cout<< " Tokencount(";
            if (a.tokenCount.intSmaller == -1) 
                std::cout<< a.tokenCount.placeSmaller;
            else 
                std::cout << a.tokenCount.intSmaller;
            std::cout<< " le ";
            if (a.tokenCount.intLarger == -1) 
                std::cout<< a.tokenCount.placeLarger;
            else 
                std::cout<< a.tokenCount.intLarger;
            std::cout << ")";
        }
        return;
    }
    else if (query->quantifier == NEG){
        std::cout << "!("; printQuery(query->first) ;std::cout <<")";
    }
    else if (query->quantifier == AND){
        std::cout << "(" ; printQuery(query->first);std::cout <<") AND ("; printQuery(query->second);std::cout<< ")";
    }
    else if (query->quantifier == OR){
        std::cout << "("; printQuery(query->first);std::cout <<") OR ("; printQuery(query->second);std::cout << ")";
    }
    else if (query->quantifier == A){
        std::cout << "A";
        printPath(query);
    }
    else if ( query->quantifier == E){
        std::cout << "E";
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

unsigned int CTLParser::lowerDepth(unsigned int a, unsigned int b){
    if (a < b)
        return a;
    else if (a == b)
        return a;
    else return b;
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
