#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "PetriNet.h"
#include <PetriParse/PNMLParser.h>
#include <PetriParse/QueryXMLParser.h>

namespace PetriEngine {
    class CodeGenerator {
        public:

        CodeGenerator(PetriNet* petriNet, MarkVal* m0, PNMLParser::InhibitorArcList placeInInhib, string statelabel, bool isReachBound, bool isPlaceBound); 
        void generateSource(bool *searchAllPaths, int query_id);
        void generateSourceForSSE();

        int inhibArc(unsigned int p, unsigned int t);

        void createQueries(string *stringQueries, int *negateResult, QueryXMLParser::Queries queries, std::vector<std::string> stateLabels);
        void generateSourceMultipleQueries(std::vector<std::string> *statelabels, int *solved, bool *searchAllPaths, int numberOfQueries);
        void printQueries(std::vector<std::string> queries, int numberOfQueries);
        
        private:
        int _nplaces;
        int _ntransitions;
        PetriNet* _net;
        MarkVal* _placeInInhib;
        MarkVal* _m0;
        string _statelabel;
        PNMLParser::InhibitorArcList _inhibarcs;
        bool _isReachBound;
        bool _isPlaceBound;

        char const*sl();
    };
}

#endif  /* CODEGENERATOR_H */