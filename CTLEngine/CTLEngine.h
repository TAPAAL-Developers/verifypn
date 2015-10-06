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
    enum Assignment {
        ONE, ZERO, UNKNOWN
    };
    enum { 
                FOUND,
                NOT_FOUND,
            } result;
            
    struct Configuration {
        PetriEngine::MarkVal *marking;
        CTLTree *query;
        CTLEngine::Assignment assignment;
        //CTLEngine::Configuration successors[][];
        
    };
    struct Edge {
        CTLEngine::Configuration source;
        CTLEngine::Configuration *targets;
    };
    
    
    void search(CTLTree *queryList);
    bool readSatisfactory();
    
    
private:
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    
    //Engine functions
    bool localSmolka(Configuration v);
    void assignConfiguration(Configuration v, Assignment a);
    void successors(Configuration v, std::vector<CTLEngine::Edge> W);
    bool querySatisfied;
    CTLEngine::Configuration createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query);
    
    //Debug functions
    void pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal* initialmarking);
};

#endif	/* CTLENGINE_H */

