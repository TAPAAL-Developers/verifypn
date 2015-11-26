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

enum Quantifier { AND = 1, OR, A, E, NEG, EMPTY = -1 };
enum Path { G = 1, F, X, U, pError = -1 };

struct Cardinality {
    int intSmaller;
    int placeSmaller;
    int intLarger;
    int placeLarger;
};

struct Fireability{
    int sizeofdenpencyplaces;
    Cardinality* denpencyplaces;
};

struct Atom {
    bool isFireable;
    int firesize;
    Fireability* fireset;
    Cardinality tokenCount;
};

struct CTLTree {
  Quantifier quantifier;
  Path path;
  CTLTree *first;
  CTLTree *second;
  Atom a;
} ;

struct CTLFormula {
    char* Name;
    bool Result;
    std::vector<std::string>* Techniques;
    CTLTree* Query;

    std::string boolToString(){
        if(Result)
            return " T ";
        else if (!Result)
            return " F ";
        else
            return " ERROR ";
    }

    //MCC2015 result printer
    void pResult(){
        std::cout << "FORMULA "
                  << Name
                  << boolToString()
                  << "TECHNIQUES ";

        for(std::vector<std::string>::const_iterator iter = Techniques->begin();
            iter != Techniques->end();
            iter++ )
        {
            std::cout << *iter << " ";
        }
        std::cout << std::endl;
    }
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
    int numberoftransitions;
    CTLTree* xmlToCTLquery(rapidxml::xml_node<> * root);
    Path setPathOperator(rapidxml::xml_node<> * root, bool isA, bool isE);
    
    bool charEmpty(char *query);
    void printPath(CTLTree *query);
    
};

#endif	/* CTLPARSER_H */

