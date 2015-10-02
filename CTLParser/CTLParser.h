/* 
 * File:   CTLParser.h
 * Author: moss
 *
 * Created on September 30, 2015, 10:39 AM
 */

#ifndef CTLPARSER_H
#include "rapidxml-1.13/rapidxml.hpp"
#define	CTLPARSER_H

enum Quantifier { AND, OR, A = 99, E, NEG };
enum Path { G, F, X, U, pError = -1 };

struct Atom {
    int i;
};

struct CTLquery {
  Quantifier quantifier;
  Path path;
  bool isatom;
  union QueryNode *tail;
  
} ;



union QueryNode {
    CTLquery ctlquery;
    Atom atom;
};


class CTLParser {
public:
    CTLParser();
    CTLParser(const CTLParser& orig);
    virtual ~CTLParser();
    void ParseXMLQuery(std::vector<char> buffer, QueryNode **queryList);
private:
    QueryNode* xmlToCTLquery(rapidxml::xml_node<> * root);
    Path setPathOperator(rapidxml::xml_node<> * root);
    
};

#endif	/* CTLPARSER_H */

