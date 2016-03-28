/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder.cpp
 * Author: mossns
 * 
 * Created on March 28, 2016, 11:38 AM
 */

#include "Encoder.h"

Encoder::Encoder() {
}

Encoder::Encoder(const Encoder& orig) {
}

Encoder::~Encoder() {
}

void Encoder::EncodeOneSafeMarking(const ctl::Marking* marking){
    size_t * buffer = (size_t*)calloc(marking->Length() / 8, sizeof(char));
}