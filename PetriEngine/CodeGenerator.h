#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "PetriNet.h"
#include <PetriParse/PNMLParser.h>

using namespace std;

namespace PetriEngine {
    class CodeGenerator {
        public:
        CodeGenerator(PetriNet* petriNet, MarkVal* m0, PNMLParser::InhibitorArcList placeInInhib, string statelabel); 
        void generateSource();
        int inhibArc(unsigned int p, unsigned int t);
        
        private:
        int _nplaces;
        int _ntransitions;
        PetriNet* _net;
        MarkVal* _placeInInhib;
        MarkVal* _m0;
        string _statelabel;
        PNMLParser::InhibitorArcList _inhibarcs;

        char const*sl();
    };
}

#endif  /* CODEGENERATOR_H */
