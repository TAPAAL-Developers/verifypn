/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLQuery.h
 * Author: Søren Moss Nielsen
 *
 * Created on April 22, 2016, 7:57 AM
 */

#ifndef CTLQUERY_H
#define CTLQUERY_H
#include "CTLParser.h"

enum CTLType {PATHQEURY = 1, LOPERATOR = 2, EVAL = 3, TYPE_ERROR = -1};

class CTLQuery {
    
public:
    CTLQuery(Quantifier q, Path p, bool isAtom, std::string atom_str);
    CTLQuery(const CTLQuery& orig);
    virtual ~CTLQuery();
    
    int Id;
    int Depth;
    
    CTLType GetQueryType();
    CTLQuery* GetFirstChild();
    CTLQuery* GetSecondChild();
    void SetFirstChild(CTLQuery *q);
    void SetSecondChild(CTLQuery *q);
    std::string ToSTring();
    
    Quantifier GetQuantifier();
    Path GetPath();
    std::string GetAtom();
    
private:
    bool _hasQuantifier;
    bool _hasPath;
    bool _hasAtom;
    Quantifier _q;
    Path _path;
    std::string _a;
    
    CTLQuery* _firstchild;
    CTLQuery* _secondchild;
    

};

#endif /* CTLQUERY_H */

