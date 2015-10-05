/* 
 * File:   CTLEngine.h
 * Author: mossns
 *
 * Created on September 11, 2015, 9:05 AM
 */

#ifndef CTLENGINE_H
#define	CTLENGINE_H

class CTLEngine {
public:
    CTLEngine();
    CTLEngine(const CTLEngine& orig);
    virtual ~CTLEngine();
    enum { 
                FOUND,
                NOT_FOUND,
            } result;
    
    void search(PetriEngine::PetriNet* net, PetriEngine::MarkVal* marking, CTLTree *queryList[]);
    bool readSatisfactory();
    
    
private:
    bool querySatisfied;
};

#endif	/* CTLENGINE_H */

