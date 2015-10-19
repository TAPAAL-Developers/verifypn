/* 
 * File:   CTLParser.cpp
 * Author: mossns
 * 
 * Created on September 30, 2015, 10:39 AM
 */

#include <string>
#include <iostream>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <vector>
#include "rapidxml-1.13/rapidxml.hpp"
#include "CTLParser.h"

//#include "CTLquery.h"

using namespace rapidxml;

CTLParser::CTLParser() {
}

CTLParser::CTLParser(const CTLParser& orig) {
}

CTLParser::~CTLParser() {
}

void CTLParser::ParseXMLQuery(std::vector<char> buffer, CTLTree *queryList[]) {
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
    std::cout << "Name of my first node is: " << doc.first_node()->name() << "\n";
    #endif
    root_node = doc.first_node();
    
    int i = 0;
    for (xml_node<> * property_node = root_node->first_node("property"); property_node; property_node = property_node->next_sibling()) {
        xml_node<> * id_node = property_node->first_node("id"); 
        #ifdef DEBUG
        std::cout << "Property id: " << id_node->value() << "\n";
        #endif
        xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
        queryList[i] = xmlToCTLquery(formula_node->first_node());
        printQuery(queryList[i]);
        #ifdef DEBUG
        std::cout << "\n";
        #endif
        i++;
    }
}

CTLTree* CTLParser::xmlToCTLquery(xml_node<> * root) {
    CTLTree *query = (CTLTree*)malloc(sizeof(CTLTree));
    
    char *root_name = root->name();
   // std::cout << "TEST:: Running xmlToCTLquery with " << root_name << " as root\n";
    char firstLetter = root_name[0];
    
    if (firstLetter == 'a') {
        query->quantifier = A;
       // std::cout << "TEST:: Q: " << root_name << "\n";
        query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'e' ) {
        query->quantifier = E;
        //std::cout << "TEST:: Q: " << root_name << " \n";
        query->path = setPathOperator(root->first_node());
        
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
        //std::cout << "TEST:: ATOM: " << root_name << "\n";
        if (root_name[1] == 's' ) {
            query->a.isFireable = true;
            query->a.fireset = root->first_node()->value();
            //std::cout << "-----TEST:: Returning query set: " << query->a.set << "\n" << std::flush;
            return query; 
        }
        else if (root_name[1] == 'n') {
            xml_node<> * integerNode = root->first_node();
            query->a.isFireable = false;
            query->a.fireset = NULL;
           // std::cout<< "\n\n ---------------> Found Token Count ";
            if (integerNode->name()[0] == 't') {
                query->a.tokenCount.placeSmaller = integerNode->first_node()->value();
                query->a.tokenCount.intSmaller = -1;
               // std::cout<< query->a.tokenCount.placeSmaller << " should be a smaller PLACE than ";
            }
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                query->a.tokenCount.intSmaller = atoi(temp);
                query->a.tokenCount.placeSmaller = NULL;
              //  std::cout<< query->a.tokenCount.intSmaller << " should be a smaller INTEGER-CONTANT than ";
            }
            
            integerNode = integerNode->next_sibling();
            
            if (integerNode->name()[0] == 't') {
                query->a.tokenCount.placeLarger = integerNode->first_node()->value();
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
        xml_node<> * child_node = root->first_node()->first_node();
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

Path CTLParser::setPathOperator(xml_node<> * root) {
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    if (firstLetter == 'g') {
        return G;
    }
    else if (firstLetter == 'f') {
        return F;
    }
    else if (firstLetter == 'n') {
        return X;
    }
    else if (firstLetter == 'u') {
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
            std::cout << "isFireable(" << query->a.fireset <<")";
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
    int i;
    if (query == NULL) {
        return true;
    }
    return false;
}