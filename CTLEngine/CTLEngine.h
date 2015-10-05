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
    CTLEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]);
    CTLEngine(const CTLEngine& orig);
    virtual ~CTLEngine();
    enum { 
                FOUND,
                NOT_FOUND,
            } result;
    
    void search(CTLTree *queryList[]);
    bool readSatisfactory();
    
    
private:
    bool querySatisfied;
    void pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal* initialmarking, CTLTree *queryList[]);
    bool checkCurrentState(PetriEngine::MarkVal initialmarking[]);
    bool hasChildren(PetriEngine::MarkVal initialmarking[]);
    PetriEngine::MarkVal *getNextState(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[]);
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
};

#endif	/* CTLENGINE_H */

