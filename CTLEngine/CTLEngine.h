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
        ONE = 1, ZERO = 0, UNKNOWN = -1
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
        std::vector<CTLEngine::Configuration> targets;
    };

    typedef std::vector<CTLEngine::Configuration> ConfigurationsList;
    typedef ConfigurationsList::iterator confIter;
    
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
    std::vector<CTLEngine::Configuration> configlist;
    
    

    //Engine functions
    bool localSmolka(Configuration v);
    void successors(Configuration v, std::vector<CTLEngine::Edge>& W);
    bool querySatisfied;
    CTLEngine::Configuration createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query);
    int next_state(PetriEngine::MarkVal* current_m, PetriEngine::MarkVal* next_m);
    
  
    
    //Debug functions
    void pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal* initialmarking);
    void configPrinter(CTLEngine::Configuration c);
    void edgePrinter(CTLEngine::Edge e);

    
       //Helping functions
    std::vector<int> calculateFireableTransistions(PetriEngine::MarkVal m[]);
    void makeNewMarking(PetriEngine::MarkVal m[], int t, PetriEngine::MarkVal nm[]);
    bool compareMarking(PetriEngine::MarkVal *m, PetriEngine::MarkVal *m1);
    int configurationExits(PetriEngine::MarkVal *marking, CTLTree *query);
    bool evaluateQuery(PetriEngine::MarkVal *marking, CTLTree *query);
 
};

#endif	/* CTLENGINE_H */

