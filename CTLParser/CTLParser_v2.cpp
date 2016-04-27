/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   CTLParser_v2.cpp
 * Author: mossns
 * 
 * Created on April 22, 2016, 10:15 AM
 */

#include "rapidxml-1.13/rapidxml.hpp"
#include "CTLParser_v2.h"
#include "CTLQuery.h"
#include "EvaluateableProposition.h"


using namespace rapidxml;

CTLParser_v2::CTLParser_v2() {
}

CTLParser_v2::CTLParser_v2(const CTLParser_v2& orig) {
}

CTLParser_v2::~CTLParser_v2() {
}

std::string CTLParser_v2::QueryToString(CTLQuery* query){
    if(query->GetQuantifier() == EMPTY && query->GetPath() == pError){
        return query->GetAtom();
    }
    else if (query->GetPath() == pError){
        Quantifier q = query->GetQuantifier();
        if (q == NEG){
            return query->ToSTring() + QueryToString(query->GetFirstChild());
        }
        else if (q == AND || q == OR){
            return QueryToString(query->GetFirstChild()) + query->ToSTring() + QueryToString(query->GetSecondChild());
        }
        else assert(false);
    }
    else if(query->GetQuantifier() == A || query->GetQuantifier() == E){
        if(query->GetPath() == U){
            return query->ToSTring() + "(" + QueryToString(query->GetFirstChild()) + ")(" + QueryToString(query->GetSecondChild()) + ")";
        }
        else{
            return query->ToSTring() + QueryToString(query->GetFirstChild());
        }
    }
}

void CTLParser_v2::FormatQuery(CTLQuery* query, PetriEngine::PetriNet *net){
    CTLType query_type = query->GetQueryType();
    if(query_type == EVAL){
        EvaluateableProposition *proposition = new EvaluateableProposition(query->GetAtom(), net);
        return;
    }
    else if(query_type == LOPERATOR){
        //FormatQuery(query->GetFirstChild());
        return;
    }
    else if(query_type == PATHQEURY){
        if(query->GetQuantifier() == A && query->GetPath() == G){
            assert(false);
            CTLQuery *neg_two = new CTLQuery(NEG, pError, false, "");
            neg_two->SetFirstChild(query->GetFirstChild());
            CTLQuery *ef_q = new CTLQuery(E, F, false, "");
            ef_q->SetFirstChild(neg_two);
            CTLQuery *neg_one = new CTLQuery(NEG, pError, false, "");
            neg_one->SetFirstChild(ef_q);
            query = neg_one;
        }
        else if(query->GetQuantifier() == E && query->GetPath() == G){
            CTLQuery *neg_two = new CTLQuery(NEG, pError, false, "");
            neg_two->SetFirstChild(query->GetFirstChild());
            CTLQuery *ef_q = new CTLQuery(A, F, false, "");
            ef_q->SetFirstChild(neg_two);
            CTLQuery *neg_one = new CTLQuery(NEG, pError, false, "");
            neg_one->SetFirstChild(ef_q);
            query = neg_one;
            assert(false);
        }
        //FormatQuery(query->GetFirstChild());
        return;
    }
}

CTLQuery * CTLParser_v2::ParseXMLQuery(std::vector<char> buffer, int query_number) {
    #ifdef DEBUG
    std::cout << "Creating doc\n" << std::flush;
    #endif
    xml_document<> doc;
    xml_node<> * root_node;
    
    #ifdef DEBUG
    std::cout << "Size of Path enum: " << sizeof(Path)*8 <<"\n";
    #endif
    doc.parse<0>(&buffer[0]);
    

#ifdef DEBUG
    std::cout << "First node id: " << doc.first_node()->name() << std::endl;
#endif

    root_node = doc.first_node();

#ifdef Analysis
    std::cout << "\nAnalysis:: Queries:" << std::endl;
#endif
    int i = 1;
    for (xml_node<> * property_node = root_node->first_node("property"); property_node; property_node = property_node->next_sibling()) {
        if(i == query_number){
            xml_node<> * id_node = property_node->first_node("id");
            xml_node<> * formula_node = id_node->next_sibling("description")->next_sibling("formula");
            CTLQuery * query = xmlToCTLquery(formula_node->first_node());
            std::cout<<QueryToString(query)<<std::endl;
            return query;
        }
        i++;
    }
    assert(false);
}

CTLQuery* CTLParser_v2::xmlToCTLquery(xml_node<> * root) {
    char *root_name = root->name();
    char firstLetter = root_name[0];
    CTLQuery *query;
    if (firstLetter == 'a') {
        //Construct all-paths
        query = new CTLQuery(A, getPathOperator(root), false, "");
        assert(query->GetQuantifier() == A);
    }
    else if (firstLetter == 'e' ) {
        //Construct exists-path
        query = new CTLQuery(E, getPathOperator(root), false, "");
        assert(query->GetQuantifier() == E);
    }
    else if (firstLetter == 'n' ) {
        //Construct negation
        query = new CTLQuery(NEG, pError, false, "");
        assert(query->GetQuantifier() == NEG);
    }
    else if (firstLetter == 'c' ) {
        //Construct conjunction
        query = new CTLQuery(AND, pError, false, "");
        assert(query->GetQuantifier() == AND);
    }
    else if (firstLetter == 'd' ) {
        //Construct disjunction
        query = new CTLQuery(OR, pError, false, "");
        assert(query->GetQuantifier() == OR);
    }
    else if (firstLetter == 'i' ) {
        //Construct Atom
        std::string atom_str = "";
        if (root_name[1] == 's' ){
            //Fireability Query File
            atom_str = root->name();
            atom_str = atom_str + "is-Fireable(";
            root = root->first_node();
            atom_str = atom_str + root->value();
            for (xml_node<> * transition_node = root->next_sibling(); transition_node; transition_node = transition_node->next_sibling()) {
                atom_str = atom_str + ", " + transition_node->value();
            }
            atom_str = atom_str + ")";
        }
        else if (root_name[1] == 'n') {
            //Cardinality Query File
            std::string loperator = root->name();
            xml_node<> * par_node = root->first_node();
            std::string first = parsePar(par_node);
            par_node = par_node->next_sibling();
            std::string second = parsePar(par_node);
            
            loperator = loperator_sym(loperator);
            
            atom_str = first + loperator + second;
            
        }
        else assert(false);
        query = new CTLQuery(EMPTY, pError, true, atom_str);
        query->Depth = 0;
        return query;
    }
    else assert(false);
    
    if (query->GetPath() == U) {
        xml_node<> * child_node = root->first_node()->first_node();
        query->SetFirstChild(xmlToCTLquery(child_node->first_node()));
        child_node = child_node->next_sibling();
        query->SetSecondChild(xmlToCTLquery(child_node->first_node()));
        
        query->Depth = (max_depth(query->GetFirstChild()->Depth, query->GetSecondChild()->Depth)) + 1;
    }
    else if (query->GetQuantifier() == AND || query->GetQuantifier() == OR) {
        xml_node<> * child_node = root->first_node();
        query->SetFirstChild(xmlToCTLquery(child_node));
        child_node = child_node->next_sibling();
        query->SetSecondChild(xmlToCTLquery(child_node));
        
        query->Depth = (max_depth(query->GetFirstChild()->Depth, query->GetSecondChild()->Depth)) + 1;
    }
    else if (query->GetQuantifier() == NEG) {
        query->SetFirstChild(xmlToCTLquery(root->first_node()));
        query->Depth = query->GetFirstChild()->Depth + 1;
    }
    else if (query->GetPath() == pError) {
        assert(false);
    }
    else {
        query->SetFirstChild(xmlToCTLquery(root->first_node()->first_node()));
        query->Depth = query->GetFirstChild()->Depth + 1;
    }
    
    return query;
}

Path CTLParser_v2::getPathOperator(xml_node<> * quantifyer_node){
    char path_firstLetter = quantifyer_node->first_node()->name()[0];
    if(path_firstLetter == 'f')
        return F;
    else if (path_firstLetter == 'g')
        return G;
    else if (path_firstLetter == 'n')
        return X;
    else if (path_firstLetter == 'u')
        return U;
    else assert(false);
}

std::string CTLParser_v2::parsePar(xml_node<> * parameter){
    std::string parameter_str = parameter->name();
    parameter_str = parameter_str + "(";
    if (parameter->name()[0] == 't'){
        parameter_str = parameter_str + parameter->first_node()->value() + ")";
    }
    else if(parameter->name()[0] == 'i'){
        parameter_str = parameter_str + parameter->value() + ")";
    }
    else assert(false);
    return parameter_str;
}

int CTLParser_v2::max_depth(int a, int b){
    if(a < b) return b; return a;
}

std::string CTLParser_v2::loperator_sym(std::string loperator){
    if(loperator.compare("integer-le") == 0){
        return " le ";
    }
    else return " " + loperator + " ";
}

CTLQuery * CTLParser_v2::CopyQuery(CTLQuery *source){
    if(source->GetQueryType() == EVAL){
        return new CTLQuery(EMPTY, pError, true, source->GetAtom());
    }
    else if(source->GetQueryType() == LOPERATOR){
        CTLQuery *dest = new CTLQuery(source->GetQuantifier(), pError, false, "");
        if(source->GetQuantifier() != NEG){
            dest->SetFirstChild(CopyQuery(source->GetFirstChild()));
            dest->SetSecondChild(CopyQuery(source->GetSecondChild()));
        }
        else {
            dest->SetFirstChild(CopyQuery(source->GetFirstChild()));
        }
        return dest;
    }
    else if(source->GetQueryType() == PATHQEURY){
        CTLQuery *dest = new CTLQuery(source->GetQuantifier(), source->GetPath(), false, "");
        if(source->GetPath() == U){
            dest->SetFirstChild(CopyQuery(source->GetFirstChild()));
            dest->SetSecondChild(CopyQuery(source->GetSecondChild()));
        }
        else{
            dest->SetFirstChild(CopyQuery(source->GetFirstChild()));
        }
        return dest;
    }
    else assert(false && "ERROR::: Copying query failed");
}