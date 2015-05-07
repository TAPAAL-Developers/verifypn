/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011-2015  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>
 *                          Jakob Dyhr <>
 *                          Mads Johannsen <mjohan12@student.aau.dk>
 *                          Isabella Kaufmann <ikaufm12@student.aau.dk>
 *                          Søren Moss Nielsen <smni12@student.aau.dk>
 *
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <PetriParse/PNMLParser.h>

#include <stdio.h>
#include <stdlib.h>
#include <PetriEngine/PetriNetBuilder.h>
#include <PetriEngine/PQL/PQL.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>

#include <PetriEngine/PQL/PQLParser.h>
#include <PetriEngine/PQL/Contexts.h>

#include <PetriEngine/Reachability/LinearOverApprox.h>
#include <PetriEngine/Reachability/UltimateSearch.h>
#include <PetriEngine/Reachability/RandomDFS.h>
#include <PetriEngine/Reachability/DepthFirstReachabilitySearch.h>
#include <PetriEngine/Reachability/BreadthFirstReachabilitySearch.h>
#include <PetriEngine/Reachability/LTSmin.h>
#include <math.h>

#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/QueryStringParser.h"
#include "PetriEngine/CodeGenerator.h"

#include "time.h"

#define ALFA_KOEFFICIENT 0.3
#define BETA_KOEFFICIENT 0.8


using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

/** Enumeration of return values from VerifyPN */
enum ReturnValues{
	SuccessCode	= 0,
	FailedCode	= 1,
	UnknownCode	= 2,
	ErrorCode	= 3,
        MultiFailCode   = 4
};

/** Enumeration of search-strategies in VerifyPN */
enum SearchStrategies{
	BestFS,			//LinearOverAprox + UltimateSearch
	BFS,			//LinearOverAprox + BreadthFirstReachabilitySearch
	DFS,			//LinearOverAprox + DepthFirstReachabilitySearch
	RDFS,			//LinearOverAprox + RandomDFS
	OverApprox		//LinearOverApprx
};

enum LTSminMode{
            DISABLED = 0,
            SEQ = 1,
            MC = 2
};

enum Tool{
            TPAR = 0,
            TSEQ = 1
};

#define VERSION		"1.1.1"

 int parseLine(char* line){
        int i = strlen(line);
        while (*line < '0' || *line > '9') line++;
        line[i-3] = '\0';
        i = atoi(line);
        return i;
    }
/*
int getValue(){ //Note: this value is in KB!
        FILE* file = fopen("/proc/self/status", "r");
        int result = -1;
        char line[128];


        while (fgets(line, 128, file) != NULL){
            if (strncmp(line, "VmRSS:", 6) == 0){
                result = parseLine(line);
                break;
            }
        }
        fclose(file);
        return result;
    }
*/
double diffclock(clock_t clock1, clock_t clock2){
    double diffticks = clock1 + clock2;
    double diffms = (diffticks*1000)/CLOCKS_PER_SEC;
    return diffms;
}

// Path to LTSmin run script
//string cmd = "/home/mads/verifypnLTSmin/runLTSmin.sh";
string cmd = "/home/isabella/Documents/verifypnLTSmin/runLTSmin.sh";
//string cmd = "/home/isabella/Documents/verifypnLTSmin/runLTSmin.sh";


int main(int argc, char* argv[]){
	// Commandline arguments
	bool outputtrace = false;
	int kbound = 0;
	SearchStrategies searchstrategy = BFS;
             LTSminMode ltsminMode = DISABLED;
             Tool tool = TSEQ;
	int memorylimit = 0;
	char* modelfile = NULL;
	char* queryfile = NULL;
	bool disableoverapprox = false;
  	int enablereduction = 0; // 0 ... disabled (default),  1 ... aggresive, 2 ... k-boundedness preserving
	int xmlquery = -1; // if value is nonnegative then input query file is in xml format and we verify query
						 // number xmlquery
	int numberOfCores = -1;
	bool statespaceexploration = false;
	bool printstatistics = true;
	int enableLTSmin = 0;
	int AltSS = 0;
	std::string LTSminRunning = "start - \n";
	std::vector<std::string> stateLabels;
            bool isReachBound = false;
            bool ltsminMc = false;
            bool debugging = false;
            bool verifyAllQueries = true;
            bool queryisdeadlock = false;


	//----------------------- Parse Arguments -----------------------//

	// Parse command line arguments
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--k-bound") == 0){
                        if (i==argc-1) {
                                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
				return ErrorCode;
                        }
                        if(sscanf(argv[++i], "%d", &kbound) != 1 || kbound < 0){
				fprintf(stderr, "Argument Error: Invalid number of tokens \"%s\"\n", argv[i]);
				return ErrorCode;
			}
		}else if(strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0){
			outputtrace = true;
		}else if(strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0){
			if (i==argc-1) {
                                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
				return ErrorCode;
                        }
                        char* s = argv[++i];
			if(strcmp(s, "BestFS") == 0)
				searchstrategy = BestFS;
			else if(strcmp(s, "BFS") == 0)
				searchstrategy = BFS;
			else if(strcmp(s, "DFS") == 0)
				searchstrategy = DFS;
			else if(strcmp(s, "RDFS") == 0)
				searchstrategy = RDFS;
			else if(strcmp(s, "OverApprox") == 0)
				searchstrategy = OverApprox;
			else{
				fprintf(stderr, "Argument Error: Unrecognized search strategy \"%s\"\n", s);
				return ErrorCode;
			}
                        }else if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--on-the-fly") == 0){
                            if (i==argc-1) {
                                ltsminMode = SEQ;
                            }
                            char* s = argv[++i];
                            if((strcmp(s, "seq") == 0)||(strcmp(s, "SEQ") == 0)){
                                ltsminMode = SEQ;
                            	tool = TSEQ;
                            }
                            else if((strcmp(s, "mc") == 0)||(strcmp(s, "MC") == 0)){
                                ltsminMode = MC;
                            	tool = TPAR;
                            }
                            else{
                                fprintf(stderr, "Argument Error: Unrecognized LTSmin mode \"%s\"\n", s);
                                return ErrorCode;
                            }
                            searchstrategy = OverApprox;

                            if(xmlquery == -1){
                                xmlquery = 1;
                            }}

		 else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory-limit") == 0) {
			if (i == argc - 1) {
				fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;
			}
			if (sscanf(argv[++i], "%d", &memorylimit) != 1 || memorylimit < 0) {
				fprintf(stderr, "Argument Error: Invalid memory limit \"%s\"\n", argv[i]);
				return ErrorCode;
			}
		} else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--disable-overapprox") == 0) {
			disableoverapprox = true;
		} else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--state-space-exploration") == 0) {
			statespaceexploration = true;
		} else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-statistics") == 0) {
			printstatistics = false;
		} else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xml-queries") == 0) {
			if (i == argc - 1) {
				fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;
			}
			if (sscanf(argv[++i], "%d", &xmlquery) != 1 || xmlquery <= 0) {
				fprintf(stderr, "Argument Error: Query index to verify \"%s\"\n", argv[i]);
				return ErrorCode;
			}
                                        verifyAllQueries = false;
		}else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cores") == 0) {
			if (i == argc - 1) {
				fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
				return ErrorCode;
			}
			if (sscanf(argv[++i], "%d", &numberOfCores) != 1 || xmlquery <= 0) {
				fprintf(stderr, "Argument Error: number of cores to use during verification \"%s\"\n", argv[i]);
				return ErrorCode;
			}
		}else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
				if (i == argc - 1) {
					fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
					return ErrorCode;
				}
				if (sscanf(argv[++i], "%d", &enablereduction) != 1 || enablereduction < 0 || enablereduction > 2) {
					fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
					return ErrorCode;
				}
		}else if (strcmp(argv[i], "-b") == 0 || strcmp(argv[i], "--debugging") == 0) {
                                debugging = true;
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			printf(	"Usage: verifypn [options] model-file query-file\n"
					"A tool for answering reachability of place cardinality queries (including deadlock)\n"
                                        "for weighted P/T Petri nets extended with inhibitor arcs.\n"
					"\n"
					"Options:\n"
					"  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
					"  -t, --trace                        Provide XML-trace to stderr\n"
                    "  -b, --debugging                        Print all debugging messages\n"
					"  -s, --search-strategy <strategy>   Search strategy:\n"
					"                                     - BestFS       Heuristic search (default)\n"
					"                                     - BFS          Breadth first search\n"
					"                                     - DFS          Depth first search\n"
					"                                     - RDFS         Random depth first search\n"
					"                                     - OverApprox   Linear Over Approx\n"
					"                                     - LTSmin       Using LTSmin backend\n"
					"  -m, --memory-limit <megabyte>      Memory limit for the state space search in MB,\n"
					"                                     0 for unlimited (default)\n"
					"  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
					"  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
					"  -d, --disable-over-approximation   Disable linear over approximation\n"
					"  -r, --reduction                    Enable structural net reduction:\n"
					"                                     - 0  disabled (default)\n"
					"                                     - 1  aggressive reduction\n"
					"                                     - 2  reduction preserving k-boundedness\n"
					"\n"
					"  -o, --on-the-fly                   Enable LTSmin:\n"
					"                                     - 0  disabled (default)\n"
					"                                     - 1  verify sequentially\n"
					"                                     - 2  verify parallely\n"
					"\n"
					"  -c, --cores <query index>          Run LTSmin multicore with a given number of cores\n"
					"\n"
					"Return Values:\n"
					"  0   Successful, query satisfiable\n"
					"  1   Unsuccesful, query not satisfiable\n"
					"  2   Unknown, algorithm was unable to answer the question\n"
					"  3   Error, see stderr for error message\n"
					"\n"
					"VerifyPN is a compilation of PeTe as untimed backend for TAPAAL.\n"
					"PeTe project page: <https://github.com/jopsen/PeTe>\n"
                                        "TAPAAL project page: <http://www.tapaal.net>\n");
			return 0;
		}else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0){
			printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERSION);
			printf("Copyright (C) 2011-2014 Jonas Finnemann Jensen <jopsen@gmail.com>,\n");
			printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>,\n");
			printf("                        Lars Kærlund Østergaard <larsko@gmail.com>,\n");
			printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
                        printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
			return 0;
		}else if(modelfile == NULL){
			modelfile = argv[i];
		}else if(queryfile == NULL){
			queryfile = argv[i];
		}else{
			fprintf(stderr, "Argument Error: Unrecognized option \"%s\"\n", modelfile);
			return ErrorCode;
		}
	}
	if (statespaceexploration) {
		// for state-space exploration some options are mandatory
		disableoverapprox = true;
		enablereduction = 0;
		kbound = 0;
		outputtrace = false;
		cmd += " bfs";
	}
	else cmd += " dfs";


	//----------------------- Validate Arguments -----------------------//

	//Check for model file
	if(!modelfile){
		fprintf(stderr, "CANNOT COMPUTE\n");
		return ErrorCode;
	}

	//Check for query file
	if(!modelfile && !statespaceexploration){
		fprintf(stderr, "DO_NOT_COMPETE\n");
		return ErrorCode;
	}

             if(disableoverapprox)
                searchstrategy = BFS;

	//----------------------- Open Model -----------------------//
    if(debugging) cout<<"Parsing Model and Query"<<endl;
    clock_t parse_begin = clock();
	//Load the model, begin scope to release memory from the stack
	PetriNet* net = NULL;
	MarkVal* m0 = NULL;
	VarVal* v0 = NULL;
             ReturnValues solution = UnknownCode;

             // List of inhibitor arcs and transition enabledness
             PNMLParser::InhibitorArcList inhibarcs;
             PNMLParser::TransitionEnablednessMap transitionEnabledness;
             PetriNetBuilder builder(false);
	{
		//Load the model
		ifstream mfile(modelfile, ifstream::in);
		if(!mfile){
			fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", modelfile);
			fprintf(stdout, "CANNOT_COMPUTE\n");
			return ErrorCode;
		}

		//Read everything
		stringstream buffer;
		buffer << mfile.rdbuf();

		//Parse and build the petri net
		//PetriNetBuilder builder(false);
		PNMLParser parser;
		parser.parse(buffer.str(), &builder);
		parser.makePetriNet();

        		inhibarcs = parser.getInhibitorArcs(); // Remember inhibitor arcs
		transitionEnabledness = parser.getTransitionEnabledness(); // Remember conditions for transitions

		//Build the petri net
		net = builder.makePetriNet();
		m0 = builder.makeInitialMarking();
		v0 = builder.makeInitialAssignment();

		// Close the file
		mfile.close();
	}
     //if(debugging) fprintf(stderr, "Size of model: %dKB\n", getValue());
     //if(debugging) cout<<"Size of model: "<<getValue()<<"KB\n"<<endl;

	//----------------------- Parse Query -----------------------//

	//Condition to check
	Condition* query = NULL;
	bool isInvariant = false;

	QueryXMLParser XMLparser(transitionEnabledness); // parser for XML queries
	//Read query file, begin scope to release memory
	{
		string querystring; // excluding EF and AG
		if (!statespaceexploration) {
			//Open query file
			ifstream qfile(queryfile, ifstream::in);
			if (!qfile) {
				fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", queryfile);
				fprintf(stdout, "CANNOT_COMPUTE\n");
				return ErrorCode;
			}

			//Read everything
			stringstream buffer;
			buffer << qfile.rdbuf();
			string querystr = buffer.str(); // including EF and AG

			//Parse XML the queries and querystr let be the index of xmlquery
			if (xmlquery > 0) {
				if (!XMLparser.parse(querystr)) {
					fprintf(stderr, "Error: Failed parsing XML query file\n");
					fprintf(stdout, "DO_NOT_COMPETE\n");
					return ErrorCode;
				}
				// XMLparser.printQueries();
				if (XMLparser.queries.size() < xmlquery) {
					fprintf(stderr, "Error: Wrong index of query in the XML query file\n");
					fprintf(stdout, "CANNOT_COMPUTE\n");
					return ErrorCode;
				}
				XMLparser.printQueries(xmlquery);
				fprintf(stdout, "\n");

				if (XMLparser.queries[xmlquery - 1].parsingResult == QueryXMLParser::QueryItem::UNSUPPORTED_QUERY) {
					fprintf(stdout, "The selected query in the XML query file is not supported\n");
					fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", XMLparser.queries[xmlquery-1].id.c_str());
					return ErrorCode;
				}
				// fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);
				querystr = XMLparser.queries[xmlquery - 1].queryText;
				if (!XMLparser.queries[xmlquery - 1].isPlaceBound) querystring = querystr.substr(2);
				else querystring = querystr;
				isInvariant = XMLparser.queries[xmlquery - 1].negateResult;

                                                    // Convert TAPAAL queries to LTSmin statelabels
                                                    QueryStringParser StringParser(&XMLparser, net, inhibarcs);
                                                    StringParser.generateStateLabels();
                                                    stateLabels = StringParser.getStateLabels();

                                                    if (xmlquery>0) {
                                                        //fprintf(stdout, "FORMULA %s ", XMLparser.queries[xmlquery-1].id.c_str());
                                                        fflush(stdout);
                                            }
			}

                                        else { // standard textual query
				fprintf(stdout, "Query:  %s \n", querystr.c_str());
				//Validate query type
				if (querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG") {
					fprintf(stderr, "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n", querystr.substr(0, 2).c_str());
                                                                    fprintf(stderr, "DO_NOT_COMPETE\n" );
					return ErrorCode;
				}
				//Check if is invariant
				isInvariant = querystr.substr(0, 2) == "AG";

				//Warp in not if isInvariant
				querystring = querystr.substr(2);
				if (isInvariant)
					querystring = "not ( " + querystring + " )";
			}

		//Close query file
		qfile.close();
		} else { // state-space exploration
			querystring = "false";
			isInvariant = false;
		}

		//Parse query
                        if(debugging) cout<<"querystring: "<<querystring<<endl;
		query = ParseQuery(querystring);

		if(!query){
			fprintf(stderr, "Error: Failed to parse query \"%s\"\n", querystring.c_str()); //querystr.substr(2).c_str());
                                        fprintf(stderr, "CANNOT COMPUTE\n"); //querystr.substr(2).c_str());
			return ErrorCode;
		}
	}

	// used by ltsmin for multi query
	int numberOfQueries = XMLparser.queries.size();
	bool isInvariantlist[numberOfQueries];
	Condition* querylist[numberOfQueries];
	int i;
	string querystring;
	for(i = 0; i < XMLparser.queries.size(); i++){
		string querystr = XMLparser.queries[i].queryText;
		if (!XMLparser.queries[xmlquery - 1].isPlaceBound) querystring = querystr.substr(2);
		else querystring = querystr;
		isInvariantlist[i] = XMLparser.queries[i].negateResult;
		querylist[i] = ParseQuery(querystring);
	}
        
        std::size_t found = querystring.find("deadlock");
        if(found!=std::string::npos){
            queryisdeadlock = true;
        }
            //isInvariant = isInvariantlist[xmlquery-1];

        clock_t parse_end = clock();
        if(debugging) cout<<"\nParsing time elapsed: "<<double(diffclock(parse_end,parse_begin))<<" ms\n"<<endl;


	//----------------------- Context Analysis -----------------------//
        	clock_t contextAnalysis_begin = clock();

	//Create scope for AnalysisContext
	{
		//Context analysis
		AnalysisContext context(*net);
                            if (xmlquery > 0)
                            {
                               query->analyze(context);
                            }

                        else if(ltsminMode && verifyAllQueries){
                            int i;
                            for (i = 0; i < XMLparser.queries.size(); i++) {
                                    querylist[i]->analyze(context);
                                }
                        }


		//Print errors if any
		if(context.errors().size() > 0){
			for(size_t i = 0; i < context.errors().size(); i++){
				fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
                                fprintf(stderr, "CANNOT_COMPUTE\n");
			}
			return ErrorCode;
		}
            }
        clock_t contextAnalysis_end = clock();
        if(debugging) cout<<"Context Analysis time elapsed: "<<double(diffclock(contextAnalysis_end,contextAnalysis_begin))<<" ms\n"<<endl;

//--------------------------------------------Reachability------------------------------------------------------------------
stringstream nc;
nc << numberOfCores;
string NumberOfCores = " ";
NumberOfCores += nc.str();

cmd += NumberOfCores;
if (debugging) printf("executing with the command %s\n", cmd.c_str());
	//Create reachability search strategy
	ReachabilitySearchStrategy* strategy = NULL;
	if(searchstrategy == BestFS)
		strategy = new UltimateSearch(true, kbound, memorylimit);
	else if(searchstrategy == BFS)
		strategy = new BreadthFirstReachabilitySearch(kbound, memorylimit);
	else if(searchstrategy == DFS)
		strategy = new DepthFirstReachabilitySearch(kbound, memorylimit);
	else if(searchstrategy == RDFS)
		strategy = new RandomDFS(kbound, memorylimit);
	else if(searchstrategy == OverApprox)
		strategy = NULL;
	else{
		fprintf(stderr, "Error: Search strategy selection out of range.\n");
                            fprintf(stderr, "CANNOT_COMPUTE\n");
		return ErrorCode;
	}

	// Wrap in linear over-approximation, if not disabled
	if(!disableoverapprox){
		strategy = new LinearOverApprox(strategy);
	}

	// If no strategy is provided
	if(!strategy && !(ltsminMode > 0)){
		fprintf(stderr, "No strategy what so ever!\n");
		fprintf(stderr, "CANNOT_COMPUTE\n");
		return ErrorCode;
	}

        clock_t lpsolve_begin = clock();
        ReachabilityResult result;
        int *notSatisfiable = new int[numberOfQueries];

        if (ltsminMode && !disableoverapprox){
            for(i = 0; i<numberOfQueries;i++){
        		notSatisfiable[i] = 1;
            }
        }
        else if (ltsminMode && disableoverapprox){
            for(i = 0; i<numberOfQueries;i++){
        		notSatisfiable[i] = 0;
            }
        }

        // Single query
        if(ltsminMode && !verifyAllQueries && strategy && !disableoverapprox){
            result = strategy->reachable(*net, m0, v0, querylist[xmlquery-1]);

            if(result.result() == ReachabilityResult::Unknown){
                    if(debugging) fprintf(stdout, "lpsolve: Unable to decide if query is satisfied.\n");
                    solution = UnknownCode;
            }

            else if(result.result() == ReachabilityResult::Satisfied){
                    if(debugging) fprintf(stdout, "lpsolve: Query is satisfied.\n");
                    solution = isInvariant ? FailedCode : SuccessCode;
            }

            else if(result.result() == ReachabilityResult::NotSatisfied){
                    if(debugging) fprintf(stdout, "lpsolve: Query is not satisfied.\n");
                    solution = isInvariant ? SuccessCode : FailedCode;
            }
        }

        // Multi query
        else if(ltsminMode && verifyAllQueries && strategy && !disableoverapprox){
            for (i = 0; i < numberOfQueries; i++){
                result = strategy->reachable(*net, m0, v0, querylist[i]);

                if(result.result() == ReachabilityResult::Unknown){
                    notSatisfiable[i] = 0;
                }
                else if(result.result() == ReachabilityResult::NotSatisfied){
       				if(tool == TSEQ){
                		if (isInvariantlist[i]) fprintf(stdout, "\nFORMULA %s TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ", XMLparser.queries[i].id.c_str());
                    	else if(!isInvariantlist[i]) fprintf(stdout, "\nFORMULA %s FALSE TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION \n ", XMLparser.queries[i].id.c_str());	
                	}else if(tool == TPAR){
                		if (isInvariantlist[i]) fprintf(stdout, "\nFORMULA %s TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ", XMLparser.queries[i].id.c_str());
                    	else if(!isInvariantlist[i]) fprintf(stdout, "\nFORMULA %s FALSE TECHNIQUES EXPLICIT STRUCTURAL_REDUCTION \n ", XMLparser.queries[i].id.c_str());	
                	}
                }
                else {
                    notSatisfiable[i] = 0;
                }
            }
        }
        else if (statespaceexploration && ltsminMode) {
            string dummy = "dummy";
            bool dummy1 = false;
            bool dummy2 = false;
            bool dummy3 = false;
            CodeGenerator codeGen(net, m0, inhibarcs, dummy, dummy1, dummy2, dummy3, &XMLparser);
            codeGen.generateSourceForSSE();

            clock_t LTSmin_begin = clock();

            FILE * stream;
            const int max_buffer = 256;
            char buffer[max_buffer];

            // ltsmin messages to search for
            string searchExit = "exiting now";
            string searchPins2lts = "pins2lts-seq";
            string searchS = string("Explored");
            string searchT = string("Explored");
            //string searchTMT = string("tokens in marking");
            //string searchMT = string("tokens in one Place");            
            string searchMarking = "#marking";
            string searchPlaceTokens = "#placetokens";

            string maxTokInMark = "0";
            string tokInOnePlace = "0";

            int maxTokInMarkRecords = 0;
            int tokInOnePlaceRecords = 0;
            int cores = -1;

            // verifypn messages
            string pins2ltsMessage;
            string stdmsg = "State space: ";
            string startMessage = "LTSmin has started";
            string exitMessage = "LTSmin finished";
          
            if(queryisdeadlock){
                cmd += " true";
            } else{
            cmd += " false";
            }
            if(ltsminMode == MC){ // multicore
                cmd += " -mc";
            }

            cmd += " 2>&1";

if (debugging) printf("executing with the command %s\n", cmd.c_str());


            if(ltsminMode == MC){ // multicore
            int q, m, s;
            string data;
            bool exitLTSmin = 0;
            if(debugging) printf("%s\n", startMessage.c_str());
            stream = popen(cmd.c_str(), "r");

            while (!exitLTSmin){
                if (fgets(buffer, max_buffer, stream) != NULL){
                    size_t found;
                    data = "";
                    data.append(buffer);
                    
                    // Find the amout of cores being used by LTSmin
                    if ((found = data.find("cores"))!=std::string::npos) {     
                        size_t startPos = 0;
                        string ssresult;

                            if((startPos = data.find("using")) !=std::string::npos){
                                size_t end_quote = data.find("cores", startPos + 1);
                                size_t nameLen = (end_quote - startPos) + 1;

                                ssresult = data.substr(startPos + 6, nameLen - 8);
                                cores = atoi( ssresult.c_str() );
                                if(cores < 0){
                                    fprintf(stderr, "\nCores registered incorrectly\n\n");
                                    return 0;
                                }

                            }
                    }               

                    if ((found = data.find(searchS))!=std::string::npos) {
                        size_t startPos = found;
                        string ssresult;

                            size_t end_quote = data.find("states", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 8, nameLen - 9);
                            startPos += ssresult.size();


                        string queryResult1 = string("STATE_SPACE STATES") + ssresult + " TECHNIQUES PARALLEL_PROCESSING EXPLICIT";
                        printf("%s\n", queryResult1.c_str());

                    }

                    if ((found = data.find(searchT))!=std::string::npos) {
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("states", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("transitions", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 6, nameLen - 8);
                            startPos += ssresult.size();
                        }

                        string queryResult2 = string("STATE_SPACE TRANSITIONS") + ssresult + " TECHNIQUES PARALLEL_PROCESSING EXPLICIT";
                        printf("%s\n", queryResult2.c_str());
                    }

                    // Max tokens in marking
                    if((found = data.find(searchMarking))!=std::string::npos){
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("\'", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("\'", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 1, nameLen - 2);
                            startPos += ssresult.size();
                        }

                        if(atoi( ssresult.c_str() ) > atoi( maxTokInMark.c_str() ) ){
                            maxTokInMark = ssresult;
                        }
                        
                        maxTokInMarkRecords++;                        
                    }

                    // Tokens in one place
                    if((found = data.find(searchPlaceTokens))!=std::string::npos){
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("\'", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("\'", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 1, nameLen - 2);
                            startPos += ssresult.size();
                        }

                        if(atoi( ssresult.c_str() ) > atoi( tokInOnePlace.c_str() ) ){
                            tokInOnePlace = ssresult;
                        }
                        
                        tokInOnePlaceRecords++;                        
                    }

                    // exit messages
                    if((found = data.find(searchExit)) != std::string::npos){
                        exitLTSmin = 1;
                        break;
                    }
                    if(cores > 0 && maxTokInMarkRecords >= cores && tokInOnePlaceRecords >= cores){
                        exitLTSmin = 1;
                        break;
                    }
                }
            }

            fprintf(stdout, "STATE_SPACE MAX_TOKENS_PER_MARKING %s TECHNIQUES PARALLEL_PROCESSING EXPLICIT\n", maxTokInMark.c_str());
            fprintf(stdout, "STATE_SPACE MAX_TOKENS_IN_PLACE %s TECHNIQUES PARALLEL_PROCESSING EXPLICIT\n", tokInOnePlace.c_str());

            pclose(stream);
            return 0; // We're done. No need to continue from here.
        }

        else if(ltsminMode == SEQ){

            int q, m, s;
            string data;
            bool exitLTSmin = 0;
            int results = 0;
            string searchMarking = "#marking";
            string searchPlaceTokens = "#placetokens";
            string searchS = string("levels,");
            string searchT = string("levels,");
            string searchTMT = string("tokens in marking");
            string searchMT = string("tokens in one Place");            

            if(debugging) printf("%s\n", startMessage.c_str());
            stream = popen(cmd.c_str(), "r");

            while (!exitLTSmin){
                if (fgets(buffer, max_buffer, stream) != NULL){
                    size_t found;
                    data = "";
                    data.append(buffer);

                    if ((found = data.find(searchS))!=std::string::npos) {
                        size_t startPos = found;
                        string ssresult;

                        size_t end_quote = data.find("states", startPos + 1);
                        size_t nameLen = (end_quote - startPos) + 1;
                        ssresult = data.substr(startPos + 7, nameLen - 8);
                        startPos += ssresult.size();
                        string queryResult1;

                        if(tool == TSEQ){
                        queryResult1 = string("STATE_SPACE STATES") + ssresult + " TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT";
                    	}
                        printf("%s\n", queryResult1.c_str());
                        results++;

                    }

                    if ((found = data.find(searchT))!=std::string::npos) {
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("states", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("transitions", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 6, nameLen - 8);
                            startPos += ssresult.size();
                        }
                        string queryResult2;

       					if(tool == TSEQ){
                        queryResult2 = string("STATE_SPACE TRANSITIONS -1 TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT");
                    	}
                        printf("%s\n", queryResult2.c_str());
                        results++;
                    }

                    // Max tokens in marking
                    if((found = data.find(searchMarking))!=std::string::npos){
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("\'", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("\'", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 1, nameLen - 2);

	                        if(tool == TSEQ){
                            fprintf(stdout, "STATE_SPACE MAX_TOKENS_PER_MARKING %s TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT\n", ssresult.c_str());
	                    	}
                            results++;
                        }
                    }

                    // Tokens in one place
                    if((found = data.find(searchPlaceTokens))!=std::string::npos){
                        size_t startPos = 0;
                        string ssresult;

                        if((startPos = data.find("\'", startPos)) != std::string::npos) {
                            size_t end_quote = data.find("\'", startPos + 1);
                            size_t nameLen = (end_quote - startPos) + 1;
                            ssresult = data.substr(startPos + 1, nameLen - 2);
                            if(tool == TSEQ){
                            fprintf(stdout, "STATE_SPACE MAX_TOKENS_IN_PLACE %s TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT\n", ssresult.c_str());
	                    	}
                            results++;
                        }                      
                    }

                    // exit messages
                    if((found = data.find(searchExit)) != std::string::npos){
                        exitLTSmin = 1;
                        break;
                    }
                    if(results >= 4){
                        exitLTSmin =1;
                        break;
                    }
                }
            }

            pclose(stream);

            return 0; // We're done. No need to continue from here.
        }

        } else {
            result = strategy->reachable(*net, m0, v0, query);
        }

        clock_t lpsolve_end = clock();
        if(debugging) cout<<"lpsolve time elapsed: "<<double(diffclock(lpsolve_end,lpsolve_begin))<<" ms\n"<<endl;

        //--------------------- Apply Net Reduction ---------------//

        clock_t reduction_begin = clock();
        Reducer reducer = Reducer(net);
        if (enablereduction == 1 or enablereduction == 2) {
            int i;
            MarkVal* placeInQuery = new MarkVal[net->numberOfPlaces()];
            for (size_t i = 0; i < net->numberOfPlaces(); i++) {
			placeInQuery[i] = 0;
            }
            QueryPlaceAnalysisContext placecontext(*net, placeInQuery);

            if(verifyAllQueries && !queryisdeadlock){
                MarkVal* placeInInhib = new MarkVal[net->numberOfPlaces()];
                MarkVal* transitionInInhib = new MarkVal[net->numberOfTransitions()];

                string reductionquerystr;
                bool firstAccurance = true;
                for (i = 0; i < XMLparser.queries.size(); i++){
                    if (notSatisfiable[i] == 0){
                        if(!firstAccurance)
                            reductionquerystr += " and ";
                        if(XMLparser.queries[i].isPlaceBound)
                        	reductionquerystr += XMLparser.queries[i].queryText;
                        else reductionquerystr += querylist[i]->toString();
                        firstAccurance = false;
                    }
                }
                Condition* reductionquery;
                if(!firstAccurance) {
                    reductionquery = ParseQuery(reductionquerystr);
                    reductionquery->analyze(placecontext);
                }
                else{
                	return ErrorCode;
                }
                //Reduce(net) - Multi Query
                reducer.CreateInhibitorPlacesAndTransitions(net, inhibarcs, placeInInhib, transitionInInhib);
                reducer.Reduce(net, m0, placeInQuery, placeInInhib, transitionInInhib, enablereduction);

                double removedTransitions_d = reducer.RemovedTransitions();
                double removedPlaces_d = reducer.RemovedPlaces();
                double numberPlaces_d = net->numberOfPlaces();
                double numberTransitions_d = net->numberOfTransitions();

                double reduceabilityfactor = (removedTransitions_d + removedPlaces_d) / (numberPlaces_d + numberTransitions_d);
                if (debugging) fprintf(stdout, "Reduceabilityfactor: (%f+%f)/(%f+%f)=%f\n",removedTransitions_d, removedPlaces_d, numberPlaces_d, numberTransitions_d, reduceabilityfactor);
            } else { //Reducing based on a single query
                query->analyze(placecontext);

                // Compute the places and transitions that connect to inhibitor arcs
                MarkVal* placeInInhib = new MarkVal[net->numberOfPlaces()];
                MarkVal* transitionInInhib = new MarkVal[net->numberOfTransitions()];

                // CreateInhibitorPlacesAndTransitions translates inhibitor place/transitions names to indexes
                reducer.CreateInhibitorPlacesAndTransitions(net, inhibarcs, placeInInhib, transitionInInhib);

                //reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib);
                reducer.Reduce(net, m0, placeInQuery, placeInInhib, transitionInInhib, enablereduction); // reduce the net
                //reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib);
            }
        }

        //----------------------- For reduction testing-----------------------------
        if(debugging){
            fprintf(stdout, "Removed transitions: %d\n", reducer.RemovedTransitions());
            fprintf(stdout, "Removed places: %d\n", reducer.RemovedPlaces());
            fprintf(stdout, "Applications of rule A: %d\n", reducer.RuleA());
            fprintf(stdout, "Applications of rule B: %d\n", reducer.RuleB());
            fprintf(stdout, "Applications of rule C: %d\n", reducer.RuleC());
            fprintf(stdout, "Applications of rule D: %d\n", reducer.RuleD());
        }
        //----------------------------------Mvh. Søren--------------------------

        clock_t reduction_end = clock();
        if(debugging) cout<<"Reduction time elapsed: "<<double(diffclock(reduction_end,reduction_begin))<<" ms\n"<<endl;

            //if (ltsminMode && statespaceexploration) {  /* whats the point of statespaceexploration here? */
            if (ltsminMode && !statespaceexploration) {
                clock_t codeGen_begin = clock();

                if(debugging) cout<<"Number of places: "<<net->numberOfPlaces()<<endl;
                if(debugging) cout<<"Number of transisions: "<<net->numberOfTransitions()<<endl;
                
                if(debugging) cout<<"Creating Code Generator object"<<endl;
                cout<<"query: "<<XMLparser.queries[xmlquery - 1].queryText<<endl;
                CodeGenerator codeGen(net, m0, inhibarcs, stateLabels[xmlquery - 1], XMLparser.queries[xmlquery - 1].isReachBound, XMLparser.queries[xmlquery - 1].isPlaceBound, XMLparser.queries[xmlquery - 1].quickSolve, &XMLparser);

                int numberOfQueries = XMLparser.queries.size();
                string* stringQueries = new string[numberOfQueries];

                if(ltsminMode && !verifyAllQueries){ // Generate code for single query
                    if(debugging) cout<<"Generating code for single query"<<endl;
                    codeGen.generateSource(isInvariantlist, (xmlquery - 1));
                }

                else if(ltsminMode && verifyAllQueries && !queryisdeadlock){ // Generate code for all queries
                    if(debugging) cout<<"Generating code for all queries"<<endl;
                    codeGen.generateSourceMultipleQueries(&stateLabels, notSatisfiable, isInvariantlist, numberOfQueries);
                }
                
                else if(queryisdeadlock){
                    if(debugging) cout<<"Generating code for deadlock query"<<endl;
                    codeGen.generateSource(isInvariantlist, -1);
                }

                clock_t codeGen_end = clock();
                if(debugging) cout<<"LTSmin Code Generation time elapsed: "<<double(diffclock(codeGen_end,codeGen_begin))<<" ms\n"<<endl;
            }


    //--------------------------------------------RUNNING LTSMIN---------------------------------------------------//

     if(ltsminMode && !statespaceexploration) {
         const std::vector<std::string> placeNames = net->placeNames();
         for (int i = 0; i < net->numberOfPlaces(); i++) {
             if(debugging) cout<<"Place index: "<<i<<" - Place name: "<<placeNames[i]<<endl;
        }



     clock_t LTSmin_begin = clock();

	FILE * stream;
	const int max_buffer = 256;
	char buffer[max_buffer];

	// ltsmin messages to search for
	string searchExit = "exiting now";
	string searchPins2lts = "pins2lts-seq";

	// verifypn messages
	string startMessage = "LTSmin has started";
	string exitMessage = "LTSmin finished";

            if(queryisdeadlock){
                cmd += " true";
            } else{
	        cmd += " false";
            }
            if(ltsminMode == MC){ // multicore
                cmd += " -mc";
            }

            cmd += " 2>&1";

          LTSmin ltsmin = LTSmin();
	  ReachabilityResult result;
	  if(debugging) cout<<"Starting LTSmin with the cmd: "<<cmd<<endl;
	  // verify only one query
	  if(ltsminMode && !verifyAllQueries && (solution == UnknownCode) && !statespaceexploration){
                if(debugging) cout<<"Starting LTSmin single query"<<endl;

                result = ltsmin.reachable(cmd, xmlquery-1, XMLparser.queries[xmlquery-1].id, XMLparser.queries[xmlquery-1].isPlaceBound, XMLparser.queries[xmlquery-1].isReachBound);
                if(debugging) cout<<"LTSmin has finished"<<endl;
                    if(result.result() == ReachabilityResult::Satisfied)
                        solution = isInvariant ? FailedCode : SuccessCode;
                    else if(result.result() == ReachabilityResult::NotSatisfied)
                        solution = isInvariant ? SuccessCode : FailedCode;
                    else
                        solution = UnknownCode;

	  if(debugging) printf("%s\n", exitMessage.c_str());
     } else if(ltsminMode && queryisdeadlock){
         bool deadlockFound = false;

	string searchDeadlock = "Deadlock found";
	string searchDeadlock1 = "deadlock () found";
	string exitMessage = "exiting now";
	string data = "";

	stream = popen(cmd.c_str(), "r");
	while (!deadlockFound){
	    if (fgets(buffer, max_buffer, stream) != NULL){
		size_t found;
		data.append(buffer);

		size_t startPos = 0;

	        if((startPos = data.find(searchDeadlock)) != std::string::npos) {
		    string queryResultSat = string("FORMULA ") + XMLparser.queries[0].id.c_str() + 
                    " TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
                    printf("%s\n", queryResultSat.c_str());
		    deadlockFound = true;
		    break;
     		}

     		if((startPos = data.find(searchDeadlock1)) != std::string::npos) {
	     		if(tool == TSEQ){
	     			string queryResultSat = string("FORMULA ") + XMLparser.queries[0].id.c_str() + 
	                    " TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                    printf("%s\n", queryResultSat.c_str());
			    deadlockFound = true;
			      break;
	     		}
     		}


	        if((startPos = data.find(exitMessage)) != std::string::npos) {

	        if(tool == TSEQ){

		    string queryResultNotSat = string("FORMULA ") + XMLparser.queries[0].id.c_str() + 
                    " FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
        	    printf("%s\n", queryResultNotSat.c_str());
                    break;

	        }
	        else if(tool == TPAR){

		    string queryResultNotSat = string("FORMULA ") + XMLparser.queries[0].id.c_str() + 
                    " FALSE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
        	    printf("%s\n", queryResultNotSat.c_str());
                    break;

	        }

     		}
            }
	}
    }
	  // verify all queries at once
	  else if(ltsminMode && verifyAllQueries && !statespaceexploration){
		  int q, m, s;
	              string data;

		  int ltsminVerified[numberOfQueries]; // keep track of what ltsmin has verified
		  int solved[numberOfQueries];
		  for(q = 0; q<numberOfQueries; q++){
		  	if(notSatisfiable[q] && !disableoverapprox)
		  		solved[q] = 1;
		  	else
		  		solved[q] = 0;

		  	ltsminVerified[q] = 0;
		  }


		bool exitLTSmin = 0;

                        int cores = -1;
                        int maxTokens[numberOfQueries];
                        int maxTokensRecords[numberOfQueries];
                        int satRecords[numberOfQueries];

                        for(int q = 0; q < numberOfQueries; q++){
                                maxTokens[q] = 0;
                                maxTokensRecords[q] = 0;
                                satRecords[q] = 0;
                        }

        if(false){
        	sec_cmd:
        	AltSS++;
        	size_t found1;
        	if ((found1 = cmd.find("dfs"))!=std::string::npos) {     
                size_t AltssLenght = 3;
                cmd.replace(found1, AltssLenght, "bfs");
                if (debugging) printf("New command for second attempt %s\n", cmd.c_str());
            }
        }


                    
		if(debugging) printf("%s\n", startMessage.c_str());
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
                                                size_t end_quote = data.find("cores", startPos + 1);
                                                size_t nameLen = (end_quote - startPos) + 1;

                                                ssresult = data.substr(startPos + 6, nameLen - 8);
                                                cores = atoi( ssresult.c_str() );
                                                if(cores < 0){
                                                    fprintf(stderr, "\nCores registered incorrectly\n\n");
                                                    return 0;
                                                }
                                            }
                                        if(cores < 0)
                                        cores = 1;
                                    }               

	                        for(q = 0; q<numberOfQueries; q++){

	                                stringstream ss;
	                                ss << q;
	                                string number = ss.str();
	                                string queryResultSat;
	                                string queryResultNotSat;


	                                		if(tool == TSEQ){

                                            queryResultSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
                                            queryResultNotSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                                		}
	                                		else if(tool == TPAR){

                                            queryResultSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
                                            queryResultNotSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " FALSE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                                		}


                                        if(XMLparser.queries[q].isPlaceBound){
                                            
                                            string searchPlaceBound = string("Query ") + number.c_str() + " max tokens are";
                                            string tokens;
                                            size_t startPos = 0;

                                            if((startPos = data.find(searchPlaceBound)) != std::string::npos) {
                                                if((startPos = data.find("\'", startPos)) != std::string::npos) {
                                                    size_t end_quote = data.find("\'", startPos + 1);
                                                    size_t nameLen = (end_quote - startPos) + 1;
                                                    tokens = data.substr(startPos + 1, nameLen - 2);

                                                    if(atoi( tokens.c_str() ) > maxTokens[q] ){
                                                        maxTokens[q] = atoi(tokens.c_str());
                                                    }
                                                    maxTokensRecords[q]++;
                                                }
                                            }
                                            // exit if all answers are received
                                            if(cores > 0){
                                                for(int q = 0; q < numberOfQueries; q++){
                                                    if(maxTokensRecords[q] >= cores && notSatisfiable[q] == 0)
                                                        exitLTSmin = 1;
                                                    else if(notSatisfiable[q] == 0){
                                                        exitLTSmin = 0;
                                                        continue;
                                                    }
                                                }
                                            }
                                        }
                                        else if(XMLparser.queries[q].isReachBound){
                                            string searchUnknown = string("#Query ") + number + " unable to decide.";
                                            string searchSat = string("#Query ") + number + " is satisfied.";

                                            if ((found = data.find(searchSat))!=std::string::npos) {
                                                satRecords[q]++;

                                                if(!ltsminVerified[q] && !solved[q]){
                                                    if(isInvariantlist[q]){
                                                        printf("%s\n", queryResultNotSat.c_str());
                                                    }
                                                    else if(!isInvariantlist[q])
                                                        printf("%s\n", queryResultSat.c_str());
                                                
                                                    solved[q] = 1;
                                                    ltsminVerified[q] = 1;
                                                }
                                            }

                                            else if((found = data.find(searchUnknown)) != std::string::npos){
                                                satRecords[q]++;
                                            } 

                                            if(cores > 0){
                                                for(int q = 0; q < numberOfQueries; q++){
                                                    if(satRecords[q] >= cores && notSatisfiable[q] == 0){
                                                        exitLTSmin = 1;
                                                    }
                                                    else if(notSatisfiable[q] == 0){
                                                        exitLTSmin = 0;
                                                        continue;
                                                    }
                                                }
                                            }                                               
                                        }

                                        else{

	                                string searchSat = string("#Query ") + number + " is satisfied.";
	                                string searchNotSat = string("#Query ") + number + " is NOT satisfied.";
	                                string hashtable = string("Error: hash table full!");

	                                if ((found = data.find(searchSat))!=std::string::npos && !ltsminVerified[q]) {

                                                if(isInvariantlist[q])
	                                       printf("%s\n", queryResultNotSat.c_str());
                                                else if(!isInvariantlist[q])
                                                    printf("%s\n", queryResultSat.c_str());
	                                    solved[q] = 1;
	                                    ltsminVerified[q] = 1;
	                                }
	                                else if((found = data.find(searchNotSat)) != std::string::npos && !ltsminVerified[q]){
                                                if(isInvariantlist[q])
                                                    printf("%s\n", queryResultSat.c_str());
                                                else if(!isInvariantlist[q])
                                                    printf("%s\n", queryResultNotSat.c_str());
                                                solved[q] = 1;
	                                    ltsminVerified[q] = 1;
	                                }
	                                else if((found = data.find(hashtable)) != std::string::npos && AltSS == 0){
                                             goto sec_cmd;
	                                } else if((found = data.find(hashtable)) != std::string::npos && AltSS != 0){
                                             goto end;
	                                }
                                        }
                                    }



	                        // exit messages
                        	if((found = data.find(searchExit)) != std::string::npos){
                        		//printf("%s\n", exitMessage.c_str());
                        		exitLTSmin = 1;
                        		break;
                        	}
	                    }
	                }

	                pclose(stream);

	                // evaluate results
	                for(int q = 0; q<numberOfQueries;q++){

	                	string queryResultSat;
	                    string queryResultNotSat;
	                	//EF not satisfied
	                	if(!solved[q] && !isInvariantlist[q] && !ltsminVerified[q] && !XMLparser.queries[q].isPlaceBound){
	                	if(tool == TSEQ){
	                		 	queryResultNotSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                		fprintf(stdout, "%s\n", queryResultNotSat.c_str());
	                	}
	                	else if(tool == TPAR){
	                		 	queryResultNotSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " FALSE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                		fprintf(stdout, "%s\n", queryResultNotSat.c_str());
	                	}

			       
	                	}

	                	//AG satisfied
	                	else if(!solved[q] && isInvariantlist[q] && !ltsminVerified[q] && !XMLparser.queries[q].isPlaceBound){

	                		if(tool == TSEQ){
	                			queryResultSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                		fprintf(stdout, "%s\n", queryResultSat.c_str());
	                		}
	                		else if(tool == TPAR){
	                			queryResultSat = string("FORMULA ") + XMLparser.queries[q].id.c_str() + " TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n ";
	                		fprintf(stdout, "%s\n", queryResultSat.c_str());
	                		}
	                		
	                	}
                                    else if(XMLparser.queries[q].isPlaceBound){
                                    	if(tool == TSEQ){
                                    		fprintf(stdout, "FORMULA %s %d TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", XMLparser.queries[q].id.c_str(), maxTokens[q]);
                                    	}
                                    	else if(tool == TPAR){
                                    		fprintf(stdout, "FORMULA %s %d TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", XMLparser.queries[q].id.c_str(), maxTokens[q]);
                                    	}
                                        
                                    }
	                }
	                if(debugging) printf("%s\n", exitMessage.c_str());

	  }
	  end:
            clock_t LTSmin_end = clock();
            if(debugging) cout<<"------------LTSmin Verification time elapsed: "<<double(diffclock(LTSmin_end,LTSmin_begin))<<" ms-----------\n"<<endl;

            // ----------------- Output LTSmin Result ----------------- //
            if(ltsminMode && !verifyAllQueries){
                if (xmlquery>0 && XMLparser.queries[xmlquery-1].isPlaceBound) {
                    // maybe move ltsmin output here.
                }
                else{
                    fprintf(stdout, "FORMULA %s ", XMLparser.queries[xmlquery-1].id.c_str());

                    if(solution == FailedCode){

                		if(tool == TSEQ){
                    		  fprintf(stdout, "FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
                        fprintf(stdout, "\nQuery is NOT satisfied.\n\n");
                    	}
                    	else if(tool == TPAR){
                    		  fprintf(stdout, "FALSE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
                        fprintf(stdout, "\nQuery is NOT satisfied.\n\n");
                    	}

                      
                    }

                    else if(solution == SuccessCode){

                    	if(tool == TSEQ){

                        fprintf(stdout, "TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
                        fprintf(stdout, "\nQuery is satisfied.\n\n");
                    	}
                    	else if(tool == TPAR){

                        fprintf(stdout, "TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
                        fprintf(stdout, "\nQuery is satisfied.\n\n");
                    	}

                    }

                    else
                        fprintf(stdout, "\nUnable to decide if query is satisfied\n\n");
                }

            return solution;

        }
        return 0;
    }

	//----------------------- Output Result -----------------------//

	const std::vector<std::string>& tnames = net->transitionNames();
	const std::vector<std::string>& pnames = net->placeNames();

	ReturnValues retval = ErrorCode;

/*	if (statespaceexploration) {
		retval = UnknownCode;
		unsigned int placeBound = 0;
		for(size_t p = 0; p < result.maxPlaceBound().size(); p++) {
			placeBound = std::max<unsigned int>(placeBound,result.maxPlaceBound()[p]);
		}
		fprintf(stdout,"STATE_SPACE %lli -1 %d %d TECHNIQUES EXPLICIT\n", result.exploredStates(), result.maxTokens(), placeBound);
		return retval;
	}
*/
	//Find result code
	if(!statespaceexploration){
	if(result.result() == ReachabilityResult::Unknown)
		retval = UnknownCode;
	else if(result.result() == ReachabilityResult::Satisfied)
		retval = isInvariant ? FailedCode : SuccessCode;
	else if(result.result() == ReachabilityResult::NotSatisfied)
		retval = isInvariant ? SuccessCode : FailedCode;

	//Print result
	if(retval == UnknownCode)
		fprintf(stdout, "\nUnable to decide if query is satisfied.\n\n");
	else if(retval == SuccessCode) {
		if (xmlquery>0) {

			if(tool == TSEQ){

			fprintf(stdout, "TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
			}
			else if(tool == TPAR){

			fprintf(stdout, "TRUE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
			}

		}
        fprintf(stdout, "\nQuery is satisfied.\n\n");
	} else if(retval == FailedCode) {
		if (xmlquery>0 && XMLparser.queries[xmlquery].isPlaceBound) {
			// find index of the place for reporting place bound
			for(size_t p = 0; p < result.maxPlaceBound().size(); p++) {
				if (pnames[p]==XMLparser.queries[xmlquery].placeNameForBound) {
					if(tool == TSEQ){

					fprintf(stdout, "%d TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", result.maxPlaceBound()[p]);
					}
					else if(tool == TPAR){

					fprintf(stdout, "%d TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", result.maxPlaceBound()[p]);
					}
					fprintf(stdout, "\nMaximum number of tokens in place %s: %d\n\n",XMLparser.queries[xmlquery].placeNameForBound.c_str(),result.maxPlaceBound()[p]);
                    retval = UnknownCode;
                    			break;
				}
			}
		} else {
			if (xmlquery>0) {
				if(tool == TSEQ){

				fprintf(stdout, "FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
				}
				else if(tool == TPAR){

				fprintf(stdout, "FALSE TECHNIQUES PARALLEL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n");
				}
			}
            fprintf(stdout, "\nQuery is NOT satisfied.\n\n");
		}
	}}
	//----------------------- Output Trace -----------------------//


	//--------------------------------------------------------------//

	//Print result to stderr
	if(outputtrace && result.result() == ReachabilityResult::Satisfied){
		const std::vector<unsigned int>& trace = (enablereduction==0 ? result.trace() : reducer.NonreducedTrace(net,result.trace()));
		fprintf(stderr, "Trace:\n<trace>\n");
		for(size_t i = 0; i < trace.size(); i++){
			fprintf(stderr, "\t<transition id=\"%s\">\n", tnames[trace[i]].c_str());
			for(unsigned int p = 0; p < net->numberOfPlaces(); p++){
				if(net->inArc(p, trace[i])) {
					for (int weight=1; weight<= net->inArc(p, trace[i]); weight++) {
       		                              fprintf(stderr, "\t\t<token place=\"%s\" age=\"0\"/>\n", pnames[p].c_str());
                                    }
  				}
			}
			fprintf(stderr, "\t</transition>\n");
		}
		fprintf(stderr, "</trace>\n");
	}

	//----------------------- Output Statistics -----------------------//

	if (printstatistics) {
	//Print statistics
	fprintf(stdout, "STATS:\n");
	fprintf(stdout, "\tdiscovered states: %lli\n", result.discoveredStates());
	fprintf(stdout, "\texplored states:   %lli\n", result.exploredStates());
	fprintf(stdout, "\texpanded states:   %lli\n", result.expandedStates());
	fprintf(stdout, "\tmax tokens:        %i\n", result.maxTokens());
        if (enablereduction!=0) {
                fprintf(stdout, "\nNet reduction is enabled.\n");
                fprintf(stdout, "Removed transitions: %d\n", reducer.RemovedTransitions());
                fprintf(stdout, "Removed places: %d\n", reducer.RemovedPlaces());
                fprintf(stdout, "Applications of rule A: %d\n", reducer.RuleA());
                fprintf(stdout, "Applications of rule B: %d\n", reducer.RuleB());
                fprintf(stdout, "Applications of rule C: %d\n", reducer.RuleC());
                fprintf(stdout, "Applications of rule D: %d\n", reducer.RuleD());
        }
	fprintf(stdout,"\nTRANSITION STATISTICS\n");
	for(size_t t = 0; t < result.enabledTransitionsCount().size(); t++) {
		// report how many times transitions were enabled (? means that the transition was removed in net reduction)
		if (net->isTransitionSkipped(t)) {
			fprintf(stdout,"<%s:?> ", tnames[t].c_str());
		} else {
			fprintf(stdout,"<%s:%lli> ", tnames[t].c_str(), result.enabledTransitionsCount()[t]);
		}
	}
	fprintf(stdout,"\n\nPLACE-BOUND STATISTICS\n");
	for(size_t p = 0; p < result.maxPlaceBound().size(); p++) {
		// report maximum bounds for each place (? means that the place was removed in net reduction)
		if (net->isPlaceSkipped(p)) {
			fprintf(stdout,"<%s;?> ", pnames[p].c_str());
		} else {
			fprintf(stdout,"<%s;%i> ", pnames[p].c_str(), result.maxPlaceBound()[p]);
		}
	}
	fprintf(stdout,"\n\n");
	}

	//------------------------ Return the Output Value -------------------//

	return retval;
}

