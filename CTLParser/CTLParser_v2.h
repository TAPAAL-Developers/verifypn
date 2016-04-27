/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLParser_v2.h
 * Author: mossns
 *
 * Created on April 22, 2016, 10:15 AM
 */

#ifndef CTLPARSER_V2_H
#define CTLPARSER_V2_H

#include "rapidxml-1.13/rapidxml.hpp"
#include <vector>
#include <string>
#include <stdio.h>
#include "CTLQuery.h"

class CTLParser_v2 {
public:
    CTLParser_v2();
    CTLParser_v2(const CTLParser_v2& orig);
    virtual ~CTLParser_v2();
    CTLQuery * ParseXMLQuery(std::vector<char> buffer, int query_number);
    void FormatQuery(CTLQuery* query);
    std::string QueryToString(CTLQuery* query);
private:
    CTLQuery* xmlToCTLquery(rapidxml::xml_node<> * root);
    std::string parsePar(rapidxml::xml_node<> * parameter);
    Path getPathOperator(rapidxml::xml_node<> * quantifyer_node);
    int max_depth(int a, int b);
    std::string loperator_sym(std::string loperator);
    CTLQuery * CopyQuery(CTLQuery *source);

};

#endif /* CTLPARSER_V2_H */

