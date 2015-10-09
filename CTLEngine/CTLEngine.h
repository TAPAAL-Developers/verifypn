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
    
    struct Markings{
        std::vector<int> possibleTransitions;
        int index;
        PetriEngine::MarkVal* marking;
    };

    typedef std::vector<Markings> MarkingsList;
    typedef MarkingsList::iterator mIter;
    
    void search(CTLTree *queryList);
    bool readSatisfactory();


    
    
private:
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    int _nplaces;
    int _ntransitions;

    std::vector<CTLEngine::Markings> list;
    


    //Engine functions
    bool localSmolka(Configuration v);
    void assignConfiguration(Configuration v, Assignment a);
    void successors(Configuration v, std::vector<CTLEngine::Edge> W);
    bool querySatisfied;
    CTLEngine::Configuration createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query);
    int next_state(PetriEngine::MarkVal* current_m, PetriEngine::MarkVal* next_m);
    
    //Debug functions
    void pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal* initialmarking);


       //Helping functions
    std::vector<int> calculateFireableTransistions(PetriEngine::MarkVal m[]);
    void makeNewMarking(PetriEngine::MarkVal m[], int t, PetriEngine::MarkVal nm[]);
    bool compareMarking(PetriEngine::MarkVal *m, PetriEngine::MarkVal *m1);

 
};

#endif	/* CTLENGINE_H */

