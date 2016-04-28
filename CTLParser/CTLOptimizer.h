/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLOptimizer.h
 * Author: mossns
 *
 * Created on April 28, 2016, 1:54 PM
 */

#ifndef CTLOPTIMIZER_H
#define CTLOPTIMIZER_H

#include "CTLQuery.h"

class CTLOptimizer {
public:
    CTLOptimizer();
    CTLOptimizer(const CTLOptimizer& orig);
    virtual ~CTLOptimizer();
    
    CTLQuery * Optimize(CTLQuery *query);
private:
    CTLQuery * OptimizeNegation(CTLQuery *query);
};

#endif /* CTLOPTIMIZER_H */

