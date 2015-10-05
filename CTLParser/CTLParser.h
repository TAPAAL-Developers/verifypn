/* 
 * File:   CTLParser.h
 * Author: moss
 *
 * Created on September 30, 2015, 10:39 AM
 */

#ifndef CTLPARSER_H
#include "rapidxml-1.13/rapidxml.hpp"
#define	CTLPARSER_H

enum Quantifier { AND = 1, OR, A, E, NEG };
enum Path { G = 1, F, X, U, pError = -1 };

struct Atom {
    bool isFireable;
    char *set;
};

struct CTLTree {
  Quantifier quantifier;
  Path path;
  CTLTree *first;
  CTLTree *second;
  Atom a;
} ;



class CTLParser {
public:
    CTLParser();
    CTLParser(const CTLParser& orig);
    virtual ~CTLParser();
    void ParseXMLQuery(std::vector<char> buffer, CTLTree **queryList);
private:
    CTLTree* xmlToCTLquery(rapidxml::xml_node<> * root);
    Path setPathOperator(rapidxml::xml_node<> * root);
    
};

#endif	/* CTLPARSER_H */

