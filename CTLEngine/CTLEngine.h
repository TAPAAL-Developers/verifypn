/* 
 * File:   CTLEngine.h
 * Author: mossns
 *
 * Created on September 11, 2015, 9:05 AM
 */

#ifndef CTLENGINE_H
#define	CTLENGINE_H

#include "unordered_set"

class CTLEngine {
public:
    CTLEngine(PetriEngine::PetriNet* net, PetriEngine::MarkVal initialmarking[], bool CertainZero, bool global);
    CTLEngine(const CTLEngine& orig);
    virtual ~CTLEngine();
    enum Assignment {
        CZERO = 2, ONE = 1, ZERO = 0, UNKNOWN = -1
    };
    enum { 
                FOUND,
                NOT_FOUND,
            } 
    result;
     
    struct Edge;
    struct Configuration {
        PetriEngine::MarkVal *marking;
        CTLTree *query;
        CTLEngine::Assignment *assignment;
        int mCount;
        bool shouldBeNegated;
        list<Edge> denpendencyList;
        list<Edge> *succ;
               
        bool operator==(const Configuration & rhs)const{
            if(query == rhs.query){
                if(!memcmp(marking, rhs.marking, sizeof(PetriEngine::MarkVal)*mCount)){
                    return (shouldBeNegated == rhs.shouldBeNegated);
                }

            }
            return false;
        }
    };

    struct hash{
        size_t operator() (const CTLEngine::Configuration& conf) const {
            size_t hash = 0;
            for(int i = 0; i < conf.mCount; i++){
                hash ^= (conf.marking[i] << (i*4 % (sizeof(PetriEngine::MarkVal)*8))) |
                        (conf.marking[i] >> (32 - (i*4 % (sizeof(PetriEngine::MarkVal)*8))));
            }
        }
    };

    struct Edge {
        CTLEngine::Configuration source;
        std::vector<CTLEngine::Configuration> targets;
        bool operator==(const Edge & rhs)const{
            if(source == rhs.source){
                for(int i = 0; i < targets.size(); i++){
                    if(!(targets.at(i) == rhs.targets.at(i))){
                        return false;
                    }
                } return true;
            } else return false;
        }

    };

    

    struct Markings{
        std::vector<int> possibleTransitions;
        int index;
        PetriEngine::MarkVal* marking;
    };
    

    typedef std::vector<CTLEngine::Configuration> ConfigurationsList;
    typedef ConfigurationsList::iterator confIter;
    
    typedef std::vector<Markings> MarkingsList;
    typedef MarkingsList::iterator mIter;
    
    void RunEgineTest();
    void search(CTLTree *queryList);
    bool readSatisfactory();


    
    
private:
    PetriEngine::PetriNet* _net;
    PetriEngine::MarkVal* _m0;
    int _nplaces;
    int _ntransitions;
    bool querySatisfied;
    bool _CertainZero;
    bool _global;

    std::vector<CTLEngine::Markings> list;
    std::vector<CTLEngine::Configuration> configlist;
    //std::tr1::unordered_set<CTLEngine::Configuration> uset;
    unordered_set<CTLEngine::Configuration, hash> uset;

    

    //Engine functions
    bool globalSmolka(Configuration v);
    bool localSmolka(Configuration v);
    void successors(Configuration v, std::vector<CTLEngine::Edge>& W);
    CTLEngine::Configuration createConfiguration(PetriEngine::MarkVal *marking, CTLTree *query);
    int next_state(PetriEngine::MarkVal* current_m, PetriEngine::MarkVal* next_m);
    
void assignConfiguration(Configuration v, Assignment a);
  
    
    //Debug functions
    void pNetPrinter(PetriEngine::PetriNet* net, PetriEngine::MarkVal* initialmarking);
    void configPrinter(CTLEngine::Configuration c);
    void edgePrinter(CTLEngine::Edge e);

    
       //Helping functions
    std::vector<int> calculateFireableTransistions(PetriEngine::MarkVal m[]);
    void makeNewMarking(PetriEngine::MarkVal m[], int t, PetriEngine::MarkVal nm[]);
    bool compareMarking(PetriEngine::MarkVal *m, PetriEngine::MarkVal *m1);
    bool evaluateQuery(PetriEngine::MarkVal *marking, CTLTree *query);
    int secondEvaluatedParam(PetriEngine::MarkVal *marking, CTLTree *query);
    bool calculateCZERO(CTLEngine::Edge e, std::vector<CTLEngine::Edge>& W);
 
};

#endif	/* CTLENGINE_H */

