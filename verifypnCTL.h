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
    double started() const { return _start; }
    double stopped() const { return _stop; }
    bool running() const { return _running; }
    void start() {
        _running = true;
        _start = clock();
    }
    void stop() {
        _stop = clock();
        _running = false;
    }
    double duration() const { return ( (double(_stop - _start))*1000)/CLOCKS_PER_SEC; }

    ostream &operator<<(ostream &os){
        os << duration() << " ms";
    }

    std::string toString(){
        stringstream ss;
        ss << this;
        return ss.str();
    }
};

class CTLResult {
public:
    CTLResult(std::string modelname = std::string(""), CTLQuery *q = nullptr, int qnbr = -1, int stat_level = 1) :
        query(q), modelname(modelname), query_nbr(qnbr),statistics_level(stat_level){}

    std::string modelname;
    int query_nbr = 0;
    CTLQuery *query = nullptr;
    bool answer = false;
    int result = 2;             // UnknownCode (from verifypn)

    int statistics_level = 1;  // 0 = none, 1 = duration, 2 = gather everything

    double duration;
    int number_of_markings = 0;
    int number_of_configurations = 0;
};

int verifypnCTL(PetriNet* net,
                 MarkVal* m0,
                 PNMLParser::InhibitorArcList &inhibitorarcs,
                 std::string modelname,
                 vector<CTLQuery*>& queries, int xmlquery,
                 int algorithm,
                 int strategy,
                 bool print_statistics);


#endif // VERIFYPNCTL_H
