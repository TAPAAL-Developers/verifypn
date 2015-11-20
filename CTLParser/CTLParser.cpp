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
        printQuery(queryList[i]->Query);
        std::cout << "\n";
        //#endif

        #ifdef DEBUG
        std::cout << "\n";
        #endif

        i++;
        std::cout << "Running query id: "<<i<<"\n"<<std::flush;
    }
}

CTLTree* CTLParser::xmlToCTLquery(xml_node<> * root) {
    bool isA = false;
    bool isE = false;
    CTLTree *query = (CTLTree*)malloc(sizeof(CTLTree));
    char *root_name = root->name();
    std::cout << "TEST:: Running xmlToCTLquery with " << root_name << " as root\n";
    char firstLetter = root_name[0];
    
    if (firstLetter == 'a') {
        isA = true;
        query->quantifier = A;
        std::cout << "TEST:: A\n";
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

            isAG = false;
            return query;
        }
        
    }
    else if (firstLetter == 'e' ) {
    	isE = true;
        query->quantifier = E;
        std::cout << "TEST:: E\n";
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

            isEG = false;
            std::cout<<"Made subquery for EG:"<<std::endl;
            printQuery(query);
            return query;
        }
        
    }
    else if (firstLetter == 'n' ) {
        std::cout << "TEST:: NEG\n";
        query->quantifier = NEG;
        //std::cout << "TEST:: Q: " << root_name << "\n";
        //query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'c' ) {
        std::cout << "TEST:: AND\n";
        query->quantifier = AND;
        //std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'd' ) {
        std::cout << "TEST:: OR\n";
        query->quantifier = OR;
        //std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'i' ) {
        query->quantifier = EMPTY;
        std::cout << "TEST:: ATOM: " << root_name << "\n";
        if (root_name[1] == 's' ) {
            numberoftransitions = 0;
            int i= 0;
            query->a.isFireable = true;
            //std::cout << "TEST:: Current set made" <<std::endl;
            for (xml_node<> * transition_node = root->first_node(); transition_node; transition_node = transition_node->next_sibling()) {
                numberoftransitions++;
            }
            numberoftransitions++;
            query->a.fireset = (char**)malloc(sizeof(char*)*numberoftransitions);
         
            for (xml_node<> * transition_node = root->first_node(); transition_node; transition_node = transition_node->next_sibling()) {
                int size = transition_node->value_size();
                query->a.fireset[i] = strcpy((char*)malloc(sizeof(char)*size), transition_node->value());
                //std::cout << "This would be nice to read: " << query->a.fireset[i] << " \n";
                i++;
            }
            query->a.fireset[i] = NULL;
            //std::cout << "-----TEST:: Returning query set: " << query->a.set << "\n" << std::flush;
            return query; 
        }
        else if (root_name[1] == 'n') {
            std::cout<< "\n\n"<<root->first_node()->name()<< "\n\n";
            xml_node<> * integerNode = root->first_node();
            integerNode = root->first_node();
            query->a.isFireable = false;
            query->a.fireset = NULL;
            xml_node<> * temp_node = integerNode;
            while(temp_node != 0){
                std::cout<<"Name: "<<temp_node->name()<<std::endl;
                temp_node = temp_node->parent();
            }
            std::cout<< "\n\n ---------------> Found integer-le - First attribute:\n ::Name: "<<integerNode->name()<<"\n ::Value: "<<integerNode->value()<<"\n"<<std::flush;
            if (integerNode->name()[0] == 't') {
                std::cout<<"This is from token count: "<<integerNode->value()<<std::endl;
                int size = integerNode->first_node()->value_size();
                query->a.tokenCount.placeSmaller = strcpy((char*)calloc(size, sizeof(char)*size),
                                                          integerNode->first_node()->value());
                query->a.tokenCount.intSmaller = -1;
               // std::cout<< query->a.tokenCount.placeSmaller << " should be a smaller PLACE than ";
            }
            else if (integerNode->name()[0] == 'i') {
                std::cout<<"This is from integer constant: "<<integerNode->value()<<std::endl;
                char *temp;
                temp = integerNode->value();
                query->a.tokenCount.intSmaller = atoi(temp);
                query->a.tokenCount.placeSmaller = NULL;
              //  std::cout<< query->a.tokenCount.intSmaller << " should be a smaller INTEGER-CONTANT than ";
            }
            
            integerNode = integerNode->next_sibling();
            
            if (integerNode->name()[0] == 't') {
                int size = integerNode->first_node()->value_size();
                query->a.tokenCount.placeLarger = strcpy((char*)calloc(size, sizeof(char) * size),
                                                         integerNode->first_node()->value());
                query->a.tokenCount.intLarger = -1;
              //  std::cout<< query->a.tokenCount.placeLarger << " - witch is a PLACE ";
            }
                
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                query->a.tokenCount.intLarger = atoi(temp);
                query->a.tokenCount.placeLarger = NULL;
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
        query->a.fireset = NULL;
        std::cout << "TEST:: IN U :::\n";
        xml_node<> * child_node = root->first_node()->first_node();
        std::cout << "TEST:: IN U 1.:::"<<root->first_node()->name()<<"\n";
        std::cout << "TEST:: IN U 2.:::"<<root->first_node()->first_node()->name()<<"\n";
        std::cout << "TEST:: IN U 3.:::"<<root->first_node()->first_node()->first_node()->name()<<"\n";
        query->first = xmlToCTLquery(child_node->first_node());
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node->first_node());
    }
    else if (query->quantifier == AND || query->quantifier == OR) {
        query->a.fireset = NULL;
        xml_node<> * child_node = root->first_node();
        query->first = xmlToCTLquery(child_node);
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node);
    }
    else if (query->quantifier == NEG) {
        query->a.fireset = NULL;
        query->first = xmlToCTLquery(root->first_node());
    }
    else {
        query->a.fireset = NULL;
        query->first = xmlToCTLquery(root->first_node()->first_node());
    }
    //std::cout << "TEST:: Returning " << root_name << "\n";
    return query;
}

Path CTLParser::setPathOperator(xml_node<> * root, bool isA, bool isE) {
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    if (firstLetter == 'g') {
        if(isA) { isAG = true; }
        if(isE){ isEG = true; }
        std::cout<<"TEST::: G"<<std::endl;
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
            std::cout << "isFireable(" << std::flush;
            int i = 0;
            while(query->a.fireset[i] != NULL){
                std::cout << " (" << query->a.fireset[i] << ") " << std::flush;
                i++;
            } 
            std::cout << ")" << std::flush;
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
        
        
        std::cout<<"::::::::::::::::::::::::::::::::\n:::::::: Parse Test End ::::::::\n::::::::::::::::::::::::::::::::\n"<<std::endl;
        
}