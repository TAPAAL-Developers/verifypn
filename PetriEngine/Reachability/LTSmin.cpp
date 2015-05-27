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

ReachabilityResult LTSmin::reachable(string cmd, int queryIndex, string queryId, bool isPlaceBound, bool isReachBound){
    FILE * stream;
    int max_buffer = 256;
    char buffer[max_buffer];
    string searchExit = "exiting now";
    string searchPins2lts = "pins2lts-seq";

    int q, m, s;
    string data;
    int solved = 0;
    int ltsminVerified = 0;
    bool exitLTSmin = 0;
    stringstream ss;
    ss << queryIndex;
    string number = ss.str();
    string searchSat;
    string searchNotSat;
    string tokens ="0";
    int cores = -1;
    int maxTokens = 0;
    int maxTokensRecords = 0;
    int satRecords = 0;

    stream = popen(cmd.c_str(), "r");
    while (!exitLTSmin){
        if (fgets(buffer, max_buffer, stream) != NULL){
            size_t found;
            data = "";
            data.append(buffer);

            // Find the amout of cores being used by LTSmin
            if ((found = data.find("Running"))!=std::string::npos) {
                size_t startPos = 0;
                string ssresult;

                    if((startPos = data.find("using")) !=std::string::npos){
                        size_t end_quote = data.find("core", startPos + 1);
                        size_t nameLen = (end_quote - startPos) + 1;

                        ssresult = data.substr(startPos + 6, nameLen - 8);
                        cores = atoi( ssresult.c_str() );
                        if(cores < 0){
                            fprintf(stderr, "\nCores registered incorrectly\n\n");
                            return ReachabilityResult(ReachabilityResult::Unknown);

                        }
                    }
                if(cores < 0)
                    cores = 1;
            }


            if(isPlaceBound){
                string searchPlaceBound = string("Query ") + number + " max tokens are";
                size_t startPos = 0;



                if((startPos = data.find("\'", startPos)) != std::string::npos) {
                    size_t end_quote = data.find("\'", startPos + 1);
                    size_t nameLen = (end_quote - startPos) + 1;
                    tokens = data.substr(startPos + 1, nameLen - 2);
                    startPos += tokens.size();
                }

                if((found = data.find(searchPlaceBound)) != std::string::npos){
                    if(atoi( tokens.c_str() ) > maxTokens ){
                        maxTokens = atoi( tokens.c_str());
                    }
                    ltsminVerified = 1;
                    maxTokensRecords++;
                }

                if(cores > 0 && maxTokensRecords >= cores){
                    exitLTSmin = 1;
                    break;
                }
            }
            else if(isReachBound){
                string searchUnknown = string("#Query ") + number + " unable to decide.";
                string searchSat = string("#Query ") + number + " is satisfied.";

                if ((found = data.find(searchSat))!=std::string::npos) {
                    satRecords++;
                    ltsminVerified = 1;
                    solved = 1;
                    return ReachabilityResult(ReachabilityResult::Satisfied);
                }

                else if((found = data.find(searchUnknown)) != std::string::npos){
                    satRecords++;
                }

                if(cores > 0){
                    if(satRecords >= cores){
                        exitLTSmin = 1;
                    }
                    else{
                        exitLTSmin = 0;
                        continue;
                    }
                }
            }
            else{
                searchSat = string("#Query ") + number + " is satisfied.";
                searchNotSat = string("#Query ") + number + " is NOT satisfied.";

                // check if satisfied
                if ((found = data.find(searchSat))!=std::string::npos) {
                    pclose(stream);
                    return ReachabilityResult(ReachabilityResult::Satisfied);
                }

                else if ((found = data.find(searchNotSat))!=std::string::npos) {
                    pclose(stream);
                    return ReachabilityResult(ReachabilityResult::NotSatisfied);
                }
            }
            // exit messages

            if((found = data.find(searchExit)) != std::string::npos){
                exitLTSmin = 1;
                break;
            }
        }
    }

    pclose(stream);

    if(isPlaceBound)
        fprintf(stdout, "FORMULA %s %d TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION\n", queryId.c_str(), maxTokens);

    if(isReachBound){
            return ReachabilityResult(ReachabilityResult::NotSatisfied);
    }
    return ReachabilityResult(ReachabilityResult::NotSatisfied);

}

}} // Namespaces
