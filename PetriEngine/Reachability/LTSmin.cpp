#include "LTSmin.h"

#include <list>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

namespace PetriEngine{ namespace Reachability {

ReachabilityResult LTSmin::reachable(string cmd, int queryIndex, string queryId, bool isPlaceBound){
    FILE * stream;
    int max_buffer = 256;
    char buffer[max_buffer];
    string searchExit = "exiting now";
    string searchPins2lts = "pins2lts-seq";    

    int q, m, s;
    string data;                
    int numberOfExitMessages = sizeof( searchExit ) / sizeof( searchExit[0] );
    int solved = 0;
    int ltsminVerified = 0;
    bool exitLTSmin = 0;
    stringstream ss;
    ss << queryIndex;
    string number = ss.str();
    string searchSat;

    printf("LTSmin has started.\n");
    stream = popen(cmd.c_str(), "r");
    while (!exitLTSmin){
        if (fgets(buffer, max_buffer, stream) != NULL){
            size_t found;
            data = "";
            data.append(buffer);

            if(isPlaceBound){
                string searchPlaceBound = string("Query ") + number + " max tokens are";
                string maxtokens;
                size_t startPos = 0;

                if((startPos = data.find("\'", startPos)) != std::string::npos) {
                    size_t end_quote = data.find("\'", startPos + 1);
                    size_t nameLen = (end_quote - startPos) + 1;
                    maxtokens = data.substr(startPos + 1, nameLen - 2);   
                    startPos += maxtokens.size();
                }

                string queryResultPlaceBound = string("FORMULA ") + queryId.c_str() + " = " + maxtokens.c_str() + " TECHNIQUES LTSMIN EXPLICIT STRUCTURAL_REDUCTION\n ";

                if((found = data.find(searchPlaceBound)) != std::string::npos){
                    printf("%s\n", queryResultPlaceBound.c_str());
                    ltsminVerified = 1;
                }
            }

            searchSat = string("#Query ") + number + " is satisfied.";

            // check if satisfied
            if ((found = data.find(searchSat))!=std::string::npos) {
                pclose(stream);
                return ReachabilityResult(ReachabilityResult::Satisfied);
            }

            // exit messages
        
            if((found = data.find(searchExit)) != std::string::npos){
                exitLTSmin = 1;
                break;
            }
        
        }
    }
    pclose(stream);

    return ReachabilityResult(ReachabilityResult::NotSatisfied);

}

}} // Namespaces
