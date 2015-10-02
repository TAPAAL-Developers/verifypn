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

void CTLParser::ParseXMLQuery(std::vector<char> buffer, QueryNode *queryList[]) {
    std::cout << "Creating doc\n" << std::flush;
    CTLquery ctlquery;
    xml_document<> doc;
    xml_node<> * root_node;
    
    
    std::cout << "Parsing?\n" << std::flush;
    doc.parse<0>(&buffer[0]);
    std::cout << "Name of my first node is: " << doc.first_node()->name() << "\n";
    root_node = doc.first_node();
    
    int i = 0;
    for (xml_node<> * property_node = root_node->first_node("property"); property_node; property_node = property_node->next_sibling()) {
        xml_node<> * id_node = property_node->first_node("id"); 
        std::cout << "Property id: " << id_node->value() << "\n";
        xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
        queryList[i] = xmlToCTLquery(formula_node->first_node());
        std::cout << "TEST:: Print of queryList " << queryList[i]->ctlquery.quantifier << " " << queryList[i]->ctlquery.path << " \n" << std::flush;
        i++;
    }
}

QueryNode* CTLParser::xmlToCTLquery(xml_node<> * root) {
    QueryNode *query = (QueryNode*)malloc(sizeof(QueryNode));
    
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    if (firstLetter == 'a') {
        query->ctlquery.quantifier = A;
        query->ctlquery.path = setPathOperator(root->first_node());
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'e' ) {
        query->ctlquery.quantifier = E;
        std::cout << "TEST:: Q: " << root_name << " \n";
    }
    else if (firstLetter == 'n' ) {
        query->ctlquery.quantifier = NEG;
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'c' ) {
        query->ctlquery.quantifier = AND;
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'd' ) {
        query->ctlquery.quantifier = OR;
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'i' ) {
        if (root_name[1] == 's' ) {
            
        }
        else if (root_name[1] == 'n') {
            
        }
        else {
            std::cout << "ERROR in xmlToCTLquery: Invalid atom " << root_name << "\n";
        }
    }
    else {
        std::cout << "ERROR in xmlToCTLquery: Invalid boolean operator: " << root_name << "\n";
    }
    
    CTLquery q;
    if (query->ctlquery.path == pError) {
        std::cout << "ERROR in xmlToCTLquery: !!Exiting - parse error!!\n";
        //return q;
    }
    else if (query->ctlquery.path == U) {
        
    }
    else {
        for (xml_node<> * child_node = root->first_node("property"); child_node; child_node = child_node->next_sibling()) {
            query->ctlquery.tail = xmlToCTLquery(child_node);
        }
    }
    
    return query;
}

Path CTLParser::setPathOperator(xml_node<> * root) {
    char *root_name = root->name();
    char firstLetter = root_name[0];
    
    if (firstLetter == 'g') {
        std::cout << "TEST:: P: " << root_name << "\n";
        return G;
    }
    else if (firstLetter == 'f') {
        std::cout << "TEST:: P: " << root_name << "\n";
        return F;
    }
    else if (firstLetter == 'n') {
        std::cout << "TEST:: P: " << root_name << "\n";
        return X;
    }
    else if (firstLetter == 'u') {
        std::cout << "TEST:: P: " << root_name << "\n";
        return U;
    }
    else {
        std::cout << "ERROR in setPathOperator: Invalid path operator: " << root_name << "\n";
    }
    return pError;
}

