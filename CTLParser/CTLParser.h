/* 
 * File:   CTLParser.h
 * Author: moss
 *
 * Created on September 30, 2015, 10:39 AM
 */

#ifndef CTLPARSER_H
#define	CTLPARSER_H

#include "rapidxml-1.13/rapidxml.hpp"
#include <vector>
#include <string>
#include <iostream>
#include "../PetriEngine/PetriNet.h"

enum Quantifier { AND = 1, OR = 2, A = 3, E = 4, NEG = 5, EMPTY = -1 };
enum Path { G = 1, X = 2, F = 3, U = 4, pError = -1 };

struct TokenCount{
    int sizeoftokencount;
    int* cardinality;
};

struct Cardinality {
    int intSmaller;
    TokenCount placeSmaller;
    int intLarger;
    TokenCount placeLarger;
};

struct Dependency{
    int intSmaller;
    int placeSmaller;
    int intLarger;
    int placeLarger;
};

struct Fireability{
    int sizeofdenpencyplaces;
    Dependency* denpencyplaces;
};

struct Atom {
    bool isFireable;
    int firesize;
    Fireability* fireset;
    Cardinality cardinality;
};

struct CTLTree {
  Quantifier quantifier = EMPTY;
  Path path = pError;
  CTLTree *first = nullptr;
  CTLTree *second = nullptr;
  Atom a;
  unsigned int depth;
  unsigned int max_depth;
  unsigned int id;
  bool isTemporal;
} ;

struct CTLFormula {
    char* Name;
    bool Result;
    CTLTree* Query;
};

class CTLParser {
public:
    CTLParser();
    CTLParser(PetriEngine::PetriNet* net);
    CTLParser(const CTLParser& orig);
    virtual ~CTLParser();
    void ParseXMLQuery(std::vector<char> buffer, CTLFormula **queryList);
    void printQuery(CTLTree *query);
    void RunParserTest();
private:
    PetriEngine::PetriNet* _net;
    bool isAG = false;
    bool isEG = false;
    unsigned int _id_sequence;
    int numberoftransitions;
    unsigned int lowerDepth(unsigned int a, unsigned int b);
    unsigned int higherDepth(unsigned int a, unsigned int b);
    CTLTree* xmlToCTLquery(rapidxml::xml_node<> * root);
    Path setPathOperator(rapidxml::xml_node<> * root, bool isA, bool isE);
    void createAGquery(rapidxml::xml_node<> * root, CTLTree *query);
    void createEGquery(rapidxml::xml_node<> * root, CTLTree *query);
    void constructFireAtom(CTLTree *query, rapidxml::xml_node<> *root);
    void constructCardinalityAtom(CTLTree *query, rapidxml::xml_node<> *root);
    int countNumberofDependencies(int t_index);
    void convertToCardinality(int i, CTLTree *query, int t_index);
    void constructPreOperator(rapidxml::xml_node<> * integerNode, CTLTree *query);
    void constructPostOperator(rapidxml::xml_node<> * integerNode, CTLTree *query);
    
    bool charEmpty(char *query);
    void printPath(CTLTree *query);
    
};

#endif	/* CTLPARSER_H */

