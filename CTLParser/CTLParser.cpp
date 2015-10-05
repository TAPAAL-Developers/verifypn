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
        //std::cout << "TEST:: Print of queryList " << queryList[i]->ctlquery.quantifier << " " << queryList[i]->ctlquery.path << " \n" << std::flush;
        i++;
    }
}

CTLTree* CTLParser::xmlToCTLquery(xml_node<> * root) {
    CTLTree *query = (CTLTree*)malloc(sizeof(CTLTree));
    
    char *root_name = root->name();
    std::cout << "TEST:: Running xmlToCTLquery with " << root_name << " as root\n";
    char firstLetter = root_name[0];
    
    if (firstLetter == 'a') {
        query->quantifier = A;
        std::cout << "TEST:: Q: " << root_name << "\n";
        query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'e' ) {
        query->quantifier = E;
        std::cout << "TEST:: Q: " << root_name << " \n";
        query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'n' ) {
        query->quantifier = NEG;
        std::cout << "TEST:: Q: " << root_name << "\n";
        query->path = setPathOperator(root->first_node());
        
    }
    else if (firstLetter == 'c' ) {
        query->quantifier = AND;
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'd' ) {
        query->quantifier = OR;
        std::cout << "TEST:: Q: " << root_name << "\n";
    }
    else if (firstLetter == 'i' ) {
        std::cout << "TEST:: ATOM: " << root_name << "\n";
        if (root_name[1] == 's' ) {
            query->a.isFireable = true;
            query->a.set = root->first_node()->value();
            return query;
        }
        else if (root_name[1] == 'n') {
            query->a.isFireable = false;
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
        std::cout << "TEST:: Setting binary boolean with a path " << query->path << "\n";
        xml_node<> * child_node = root->first_node()->first_node();
        query->first = xmlToCTLquery(child_node->first_node());
        std::cout << "-----TEST:: Set first child: " << child_node->first_node()->name() << "\n";
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node->first_node());
        std::cout << "-----TEST:: Set second child: " << child_node->first_node()->name() << "\n";
    }
    else if (query->quantifier == AND || query->quantifier == OR) {
        std::cout << "TEST:: Setting binary boolean without a path " << root_name << "\n";
        xml_node<> * child_node = root->first_node();
        query->first = xmlToCTLquery(child_node);
        std::cout << "-----TEST:: Set first child: " << child_node->name() << "\n";
        child_node = child_node->next_sibling();
        query->second = xmlToCTLquery(child_node);
        std::cout << "-----TEST:: Set second child: " << child_node->name() << "\n";
    }
    else {
        std::cout << "TEST:: Setting unary boolean with a path " << query->path << "\n";
        query->first = xmlToCTLquery(root->first_node()->first_node());
    }
    std::cout << "TEST:: Returning " << root_name << "\n";
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

