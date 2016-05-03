/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   EvaluateableProposition.cpp
 * Author: mossns
 * 
 * Created on April 27, 2016, 2:36 PM
 */

#include <string>
#include <stdexcept>

#include "EvaluateableProposition.h"
#include "CTLParser.h"

EvaluateableProposition::EvaluateableProposition(std::string a, PetriEngine::PetriNet *net) {
    std::cout << "ATOM " << a << std::endl;

    if(a.substr(0,2).compare("in") == 0 || a.substr(0,2).compare("to") == 0){
        _type = CARDINALITY;
        _loperator = SetLoperator(a);
        assert(_loperator != NOT_CARDINALITY);

        size_t begin = a.find('(') + 1;
        size_t end = a.find(')') - begin;
        std::string first_parameter_str = a.substr(begin, end);
        a = a.substr(a.find(')') + 1);

        begin = a.find('(') + 1;
        end = a.find(')') - begin;
        std::string second_parameter_str = a.substr(begin, end);
        _firstParameter = CreateParameter(first_parameter_str, net->placeNames(), net->numberOfPlaces());
        _secondParameter = CreateParameter(second_parameter_str, net->placeNames(), net->numberOfPlaces());
    }
    else if(a.substr(0,2).compare("is") == 0){
        _type = FIREABILITY;
        _loperator = NOT_CARDINALITY;
        size_t s_pos = a.find('(') + 1;
        size_t e_pos = a.find(')') - 1;
        assert(s_pos < e_pos);
        int fire_str_length = e_pos - s_pos + 1;
        std::string fireset_str = a.substr(s_pos, fire_str_length);
        SetFireset(fireset_str, net->transitionNames(), net->numberOfTransitions());
    }
    else{
        assert(false && "Atomic string proposed for proposition could not be parsed");
    }
    std::cout << ToString() << std::endl;
}

EvaluateableProposition::~EvaluateableProposition() {
}

PropositionType EvaluateableProposition::GetPropositionType(){
    return _type;
}

LoperatorType EvaluateableProposition::GetLoperator(){
    assert(_type == CARDINALITY && "Proposition is not a cardinality proposition");
    return _loperator;
}

std::vector<int> EvaluateableProposition::GetFireset() {
    assert(_type == FIREABILITY && "Proposition is not a fireability proposition");
    return _fireset;
}

void EvaluateableProposition::SetFireset(std::string fireset_str, std::vector<std::string> t_names, unsigned int numberof_t){
    std::string restof_firestring = fireset_str;
    while(restof_firestring.length() > 0){
        size_t position = restof_firestring.find(',');
        std::string current_t = restof_firestring.substr(0, position);
        for(int i = 0; i < numberof_t; i++){
            if (current_t.compare(t_names[i]) == 0){
                _fireset.push_back(i);
            }
        }
        if (position != -1)
            restof_firestring = restof_firestring.substr(position);
        else
            restof_firestring = "";
    }
    for ( auto f : _fireset){
        std::cout<<f<<" is id of "<< t_names[f]<<std::endl;
    }
}

CardinalityParameter* EvaluateableProposition::CreateParameter(std::string parameter_str, std::vector<std::string> p_names, unsigned int numberof_p){
    CardinalityParameter *param = new CardinalityParameter();
    std::string::size_type sz;
    try{
        param->value = std::stoi(parameter_str,&sz);
        param->isPlace = false;
    }
    catch (const std::invalid_argument& e) {
        param->isPlace = true;
        for(int i = 0; i < numberof_p; i++){
            if(parameter_str.compare(p_names[i]) == 0){
                param->value = i;
                break;
            }
        }
    }
    
    return param;
}
LoperatorType EvaluateableProposition::SetLoperator(std::string atom_str){
    std::string loperator_str = atom_str.substr(atom_str.find(')'));
    loperator_str = loperator_str.substr(0, loperator_str.find('('));
    
    /*Implement parser for logical operator*/
            return LEQ;
}

std::string EvaluateableProposition::ToString() {
    if (_type == FIREABILITY) {
        std::string fire_str = "Fireset(";
        for(auto f : _fireset){
            fire_str = fire_str + " " + std::to_string(f);
        }
        return fire_str + ")";
    }
    else if (_type == CARDINALITY){
        std::string cardi_str = "(";
        if(_firstParameter->isPlace)
            cardi_str = cardi_str + "place(" + std::to_string(_firstParameter->value) + ")";
        else
            cardi_str = cardi_str = std::to_string(_firstParameter->value);
        
        cardi_str = cardi_str + " le ";
        
        if(_secondParameter->isPlace)
            cardi_str = cardi_str + "place(" + std::to_string(_secondParameter->value) + ")";
        else
            cardi_str = cardi_str = std::to_string(_secondParameter->value);
        return cardi_str + ")";
    }
    else
        assert(false && "Proposition had no type");
}

CardinalityParameter* EvaluateableProposition::GetFirstParameter() {
    assert(_type == CARDINALITY);
    return _firstParameter;
}

CardinalityParameter* EvaluateableProposition::GetSecondParameter() {
    assert(_type == CARDINALITY);
    return _secondParameter;
}
