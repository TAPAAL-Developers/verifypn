#ifndef VERIFYPNCTL_H
#define VERIFYPNCTL_H

#include "PetriEngine/PetriNet.h"
#include "PetriParse/PNMLParser.h"

#include "CTLParser/CTLParser_v2.h"

#include <ctime>
#include <sstream>

using namespace std;
using namespace PetriEngine;

class stopwatch {
    bool _running = false;
    clock_t _start;
    clock_t _stop;

public:
    bool started() const { return _running; }
    void start() {
        _running = true;
        _start = clock();
    }
    void stop() {
        _stop = clock();
        _running = false;
    }
    double duration() const { return ((_stop - _start)*1000)/CLOCKS_PER_SEC; }

    ostream &operator<<(ostream &os){
        os << duration() << " ms";
    }

    std::string toString(){
        stringstream ss;
        ss << this;
        return ss.str();
    }
};

double verifypnCTL(PetriNet* net,
                   MarkVal* m0,
                   vector<CTLQuery*>& queries,
                   int xmlquery,
                   int algorithm,
                   int strategy,
                   vector<int> results,
                   PNMLParser::InhibitorArcList inhibitorarcs, bool print_statistics);


#endif // VERIFYPNCTL_H
