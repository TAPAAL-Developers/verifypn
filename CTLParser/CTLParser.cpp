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
    std::cout << "Creating doc\n" << std::flush;
//    CTLquery ctlquery;
    xml_document<> doc;
    xml_node<> * root_node;
    
    
    std::cout << "Parsing?\n" << std::flush;
    std::cout << "Size of Path enum: " << sizeof(Path)*8 <<"\n";
    doc.parse<0>(&buffer[0]);
    std::cout << "Name of my first node is: " << doc.first_node()->name() << "\n";
    root_node = doc.first_node();
    
    int i = 0;
    for (xml_node<> * property_node = root_node->first_node("property"); property_node; property_node = property_node->next_sibling()) {
        xml_node<> * id_node = property_node->first_node("id"); 
        std::cout << "Property id: " << id_node->value() << "\n";
        xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
        queryList[i] = xmlToCTLquery(formula_node->first_node());
        printQuery(queryList[i]);
        std::cout << "\n";
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
        //std::cout << "TEST:: ATOM: " << root_name << "\n";
        if (root_name[1] == 's' ) {
            query->a.isFireable = true;
            query->a.set = root->first_node()->value();
            //std::cout << "-----TEST:: Returning query set: " << query->a.set << "\n" << std::flush;
            return query; 
        }
        else if (root_name[1] == 'n') {
            xml_node<> * integerNode = root->first_node();
            query->a.isFireable = false;
            std::string integerExpression;
            if (integerNode->name()[0] == 't') {
                char *temp;
                temp = integerNode->first_node()->value();
                integerExpression = temp;
            }
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                integerExpression = temp;
            }
            //Set less-than between
            integerExpression = integerExpression + " le ";
            integerNode = integerNode->next_sibling();
            
            if (integerNode->name()[0] == 't') {
                char *temp;
                temp = integerNode->first_node()->value();
                integerExpression = integerExpression + temp;
            }
                
            else if (integerNode->name()[0] == 'i') {
                char *temp;
                temp = integerNode->value();
                integerExpression = integerExpression + temp;
            }
            
            query->a.set = &integerExpression[0];
            //std::cout << "-----TEST:: Returning query set: " << query->a.set << "\n" << std::flush;
            return query; 
        }
        else {
            std::cout << "ERROR in xmlToCTLquery: Invalid atom " << root_name << "\n";
        }
    }
    else {
        std::cout << "ERROR in xmlToCTLquery: Invalid boolean operator: " << root_name << "\n";
    }
    
    if (query->path == pError) {
        std::cout << "ERROR in xmlToCTLquery: !!Exiting - parse error!!\n";
        
    }
    else if (query->path == U) {
        query->a.set = NULL;
        xml_node<> * child_node = root->first_node()->first_node();
        query->first = xmlToCTLquery(child_node->first_node());
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node->first_node());
    }
    else if (query->quantifier == AND || query->quantifier == OR) {
        query->a.set = NULL;
        xml_node<> * child_node = root->first_node();
        query->first = xmlToCTLquery(child_node);
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node);
    }
    else if (query->quantifier == NEG) {
        query->a.set = NULL;
        query->first = xmlToCTLquery(root->first_node());
    }
    else {
        query->a.set = NULL;
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
        std::cout << "ERROR in setPathOperator: Invalid path operator: " << root_name << "\n";
    }
    return pError;
}

void CTLParser::printQuery(CTLTree *query) {
    if(!charEmpty(query->a.set)) {
        if(query->a.isFireable){
            std::cout << "isFireable(" << query->a.set <<")";
            return;
        }
        std::cout<< " TokenCount(" << query->a.set << ")";
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
        printQuery(query->first);
    }
    else if ( query->quantifier == E){
        std::cout << "E";
        printPath(query);
        printQuery(query->first);
    }
    else return;
}

void CTLParser::printPath(CTLTree *query) {
    if (query->path == G)
        std::cout << "G";
    else if (query->path == F)
        std::cout << "F";
    else if (query->path == U)
        {std::cout << "("; printQuery(query->first);std::cout <<") U ("; printQuery(query->second);std::cout << ")";}
    else if (query->path == X)
        std::cout << "X";
}

bool CTLParser::charEmpty(char *query) {
    int i;
    if (query == NULL) {
        return true;
    }
    return false;
}