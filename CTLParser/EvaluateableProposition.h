/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EvaluateableProposition.h
 * Author: mossns
 *
 * Created on April 27, 2016, 2:36 PM
 */

#ifndef EVALUATEABLEPROPOSITION_H
#define EVALUATEABLEPROPOSITION_H
#include <vector>
#include "PetriEngine/PetriNet.h"


enum PropositionType {CARDINALITY = 0, FIREABILITY = 1};
enum LoperatorType {NOT_CARDINALITY = -1, EQ = 0, LE = 1, LEQ = 2, GR = 3, GRQ = 4};

struct CardinalityParameter{
    bool isPlace;
    int value;
};

class EvaluateableProposition {
public:
    EvaluateableProposition(std::string a, PetriEngine::PetriNet *net);
    virtual ~EvaluateableProposition();
    PropositionType GetPropositionType();
    LoperatorType GetLoperator();
    std::vector<int> GetFireset();
    std::string ToString();
private:
    void SetFireset(std::string fireset_str, std::vector<std::string> t_names, unsigned int numberof_t);
    CardinalityParameter* CreateParameter(std::string parameter_str, std::vector<std::string> p_names, unsigned int numberof_p);
    LoperatorType SetLoperator(std::string atom_str);
    
    PropositionType _type;
    LoperatorType _loperator;
    std::vector<int> _fireset;
    CardinalityParameter* _firstParameter;
    CardinalityParameter* _secondParameter;
    
};

#endif /* EVALUATEABLEPROPOSITION_H */

