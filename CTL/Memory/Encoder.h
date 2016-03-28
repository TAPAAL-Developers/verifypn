/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Encoder.h
 * Author: mossns
 *
 * Created on March 28, 2016, 11:38 AM
 */
#include "../marking.h"
#ifndef ENCODER_H
#define ENCODER_H

class Encoder {
public:
    Encoder();
    Encoder(const Encoder& orig);
    virtual ~Encoder();
    
    void EncodeOneSafeMarking(const ctl::Marking *marking);
private:

};

#endif /* ENCODER_H */

