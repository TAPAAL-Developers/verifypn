/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLOptimizer.cpp
 * Author: mossns
 * 
 * Created on April 28, 2016, 1:54 PM
 */

#include "CTLOptimizer.h"

CTLOptimizer::CTLOptimizer() {
}

CTLOptimizer::CTLOptimizer(const CTLOptimizer& orig) {
}

CTLOptimizer::~CTLOptimizer() {
}

CTLQuery* CTLOptimizer::Optimize(CTLQuery *query) {
    query = OptimizeNegation(query);
    return query;
}

CTLQuery* CTLOptimizer::OptimizeNegation(CTLQuery *query) {
    CTLType query_type = query->GetQueryType();
    if(query_type == EVAL){
        return query;
    }
    else if (query_type == LOPERATOR){
        if(query->GetQuantifier() != NEG){
            query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
            query->SetSecondChild(OptimizeNegation(query->GetSecondChild()));
        }
        else {//Negation
            if(query->GetFirstChild()->GetQueryType() == LOPERATOR && query->GetFirstChild()->GetQuantifier() == NEG){
                query = query->GetFirstChild()->GetFirstChild();
                if(query->GetQueryType() == EVAL){
                    return query;
                }
                else if(query->GetQueryType() == LOPERATOR){
                    if(query->GetQuantifier() != NEG){
                        query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
                        query->SetSecondChild(OptimizeNegation(query->GetSecondChild()));
                    }
                    else{
                        query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
                    }
                }
                else if (query->GetQueryType() == PATHQEURY){
                    if (query->GetPath() == U){
                        query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
                        query->SetSecondChild(OptimizeNegation(query->GetSecondChild()));
                    }
                    else{
                        query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
                    }
                }
            }
        }
    }
    else if (query_type == PATHQEURY){
        if (query->GetPath() == U){
            query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
            query->SetSecondChild(OptimizeNegation(query->GetSecondChild()));
        }
        else{
            query->SetFirstChild(OptimizeNegation(query->GetFirstChild()));
        }
    }
    return query;
}
