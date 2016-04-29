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

struct QueryMeta{
    int numberof_queries;
    std::string *model_name;
    
};

class CTLParser_v2 {
public:
    CTLParser_v2();
    CTLParser_v2(const CTLParser_v2& orig);
    virtual ~CTLParser_v2();
    CTLQuery * ParseXMLQuery(std::vector<char> buffer, int query_number);
    CTLQuery* FormatQuery(CTLQuery* query, PetriEngine::PetriNet *net);
    std::string QueryToString(CTLQuery* query);
    QueryMeta * GetQueryMetaData(std::vector<char> buffer);
private:
    CTLQuery* xmlToCTLquery(rapidxml::xml_node<> * root);
    std::string parsePar(rapidxml::xml_node<> * parameter);
    Path getPathOperator(rapidxml::xml_node<> * quantifyer_node);
    int max_depth(int a, int b);
    std::string loperator_sym(std::string loperator);
    CTLQuery * CopyQuery(CTLQuery *source);
    
    void FillAtom(CTLQuery* query, PetriEngine::PetriNet *net);
    CTLQuery* ConvertAG(CTLQuery* query);
    CTLQuery* ConvertEG(CTLQuery* query);
    CTLQuery* TemporalSetting(CTLQuery* query);
    

};

#endif /* CTLPARSER_V2_H */

