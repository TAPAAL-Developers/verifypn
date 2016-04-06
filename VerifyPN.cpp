/* PeTe - Petri Engine exTremE
 * Copyright (C) 2011-2015  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>
 * CTL Extension
 *                          Isabella Kaufmann <ikaufm12@student.aau.dk>
 *                          Lasse Steen Jensen <lasjen12@student.aau.dk>
 *                          Søren Moss Nielsen <smni12@student.aau.dk>
 *                          Jiri Srba <srba.jiri@gmail.com>
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
#include <PetriEngine/PetriNetBuilder.h>
#include <PetriEngine/PQL/PQL.h>
#include <string>
#include <vector>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <assert.h>

#include <time.h>

#include <PetriEngine/PQL/PQLParser.h>
#include <PetriEngine/PQL/Contexts.h>

#include <PetriEngine/Reachability/LinearOverApprox.h>
#include <PetriEngine/Reachability/UltimateSearch.h>
#include <PetriEngine/Reachability/RandomDFS.h>
#include <PetriEngine/Reachability/DepthFirstReachabilitySearch.h>
#include <PetriEngine/Reachability/BreadthFirstReachabilitySearch.h>

#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"


///CTL includes
#include "CTLParser/CTLParser.h"

#include "CTL/FixedPointAlgorithm.h"
#include "CTL/CertainZeroFPA.h"
#include "CTL/LocalFPA.h"

#include "CTL/DependencyGraph.h"
#include "CTL/OnTheFlyDG.h"

#include "CTL/SearchStrategies/AbstractSearchStrategy.h"
#include "CTL/SearchStrategies/DepthFirstSearch.h"

using namespace std;
using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

/** Enumeration of return values from VerifyPN */
enum ReturnValues{
	SuccessCode	= 0,
	FailedCode	= 1,
	UnknownCode	= 2,
	ErrorCode	= 3
};

/** Enumeration of search-strategies in VerifyPN */
enum SearchStrategies{
	BestFS,			//LinearOverAprox + UltimateSearch
	BFS,			//LinearOverAprox + BreadthFirstReachabilitySearch
	DFS,			//LinearOverAprox + DepthFirstReachabilitySearch
	RDFS,			//LinearOverAprox + RandomDFS
    OverApprox,		//LinearOverApprx
};

enum CtlAlgorithm {
    LOCAL = 0,
    CZERO =1
};

bool printstatistics;

double diffclock(clock_t clock1, clock_t clock2){
    double diffticks = clock1 - clock2;
    double diffms = (diffticks*1000)/CLOCKS_PER_SEC;
    return diffms;
}

void getQueryPlaces(vector<string> *QueryPlaces, CTLTree* current, PetriNet *net){
    if(current->depth == 0){
        if(current->a.isFireable){
            int i = 0;
            for(i = 0; i < current->a.firesize; i++){
                int j = 0;
                for (j = 0; j < current->a.fireset->sizeofdenpencyplaces; j++){
                    const char *pname_c = net->placeNames()[current->a.fireset[i].denpencyplaces[j].placeLarger].c_str();
                    string pname = pname_c;
                    QueryPlaces->insert(QueryPlaces->end(), pname);
                }
            }
        }
        else {
            int i = 0;
            string lname, sname;
            for (i = 0; i < current->a.cardinality.placeLarger.sizeoftokencount; i++){
                const char *lname_c = net->placeNames()[current->a.cardinality.placeLarger.cardinality[i]].c_str();
                lname += lname_c;
            }
            for(i = 0; i < current->a.cardinality.placeSmaller.sizeoftokencount; i++){
                const char *sname_c = net->placeNames()[current->a.cardinality.placeSmaller.cardinality[i]].c_str();
                sname += sname_c;
            }
            QueryPlaces->insert(QueryPlaces->end(), lname);
            QueryPlaces->insert(QueryPlaces->end(), sname);
        }
    }
    else if (current->quantifier == AND || current->quantifier == OR || current->path == U){
        getQueryPlaces(QueryPlaces, current->first, net);
        getQueryPlaces(QueryPlaces, current->second, net);
    }
    else {
        getQueryPlaces(QueryPlaces, current->first, net);
    }
}

void search_ctl_query(PetriNet* net,
                      MarkVal* m0,
                      CTLFormula *queryList[],
                      int t_xmlquery,
                      CtlAlgorithm t_algorithm,
                      SearchStrategies t_strategy,
                      ReturnValues result[], 
                      PNMLParser::InhibitorArcList inhibitorarcs) {

    ctl::FixedPointAlgorithm *algorithm;
    ctl::AbstractSearchStrategy *strategy;
    ctl::DependencyGraph *graph = new ctl::OnTheFlyDG(net, m0, inhibitorarcs);

    //Determine Fixed Point Algorithm
    if(t_algorithm == LOCAL){
        algorithm = new ctl::LocalFPA();
    }
    else if(t_algorithm == CZERO){
        algorithm = new ctl::CertainZeroFPA();
    }
    else{
        fprintf(stderr, "Error: unknown ctl algorithm");
        exit(EXIT_FAILURE);
    }

    //Determine Search Strategy
    if(t_strategy == DFS){
        strategy = new ctl::DepthFirstSearch();
    }
    else{
        fprintf(stderr, "Error: unsupported ctl search strategy");
        exit(EXIT_FAILURE);
    }

    int configCount = 0, markingCount = 0;
    bool res;

    if(t_xmlquery > 0){
        clock_t individual_search_begin = clock();

        graph->initialize(*(queryList[t_xmlquery - 1]->Query));
        res = algorithm->search(*graph, *strategy);

        configCount = graph->configuration_count();
        markingCount = graph->marking_count();
        queryList[t_xmlquery - 1]->Result = res;

        clock_t individual_search_end = clock();
        if (printstatistics) {
        	cout<<":::TIME::: Search elapsed time for query "<< t_xmlquery - 1 <<": "<<double(diffclock(individual_search_end,individual_search_begin))<<" ms"<<endl;
        	cout<<":::DATA::: Configurations: " << configCount << " Markings: " << markingCount << endl;
	}
        if (res)
            result[t_xmlquery - 1] = SuccessCode;
        else if (!res)
            result[t_xmlquery - 1] = FailedCode;
        else result[t_xmlquery - 1] = ErrorCode;

        queryList[t_xmlquery - 1]->pResult();
    }
    else{
        for (int i = 0; i < 16 ; i++) {
            clock_t individual_search_end, individual_search_begin;
            individual_search_begin = clock();

            graph->initialize(*(queryList[i]->Query));
            res = algorithm->search(*graph, *strategy);

            individual_search_end = clock();

            strategy->clear();
            configCount = graph->configuration_count();
            markingCount = graph->marking_count();
            queryList[i]->Result = res;

            if (printstatistics) { 
	    	cout<<":::TIME::: Search elapsed time for query "<< i <<": "<<double(diffclock(individual_search_end,individual_search_begin))<<" ms"<<endl;
            	cout<<":::DATA::: Configurations: " << configCount << " Markings: " << markingCount << endl;
	    }

            if (res)
                result[i] = SuccessCode;
            else if (!res)
                result[i] = FailedCode;
            else result[i] = ErrorCode;
            queryList[i]->pResult();
            cout << endl;
        }
   }
}

#define VERSION		"2.0.0"

int main(int argc, char* argv[]){
	// Commandline arguments
	bool outputtrace = false;
	int kbound = 0;
	SearchStrategies searchstrategy = BestFS;
	int memorylimit = 0;
    char* modelfile = NULL;
	char* queryfile = NULL;
	bool disableoverapprox = false;
  	int enablereduction = 0; // 0 ... disabled (default),  1 ... aggresive, 2 ... k-boundedness preserving
	int xmlquery = -1; // if value is nonnegative then input query file is in xml format and we verify query 
						 // number xmlquery
	bool statespaceexploration = false;
	bool printstatistics = true;
        
    //CTL variables
    //TODO Sort out when done
    bool isCTLlogic = false;
    CtlAlgorithm ctl_algorithm;
//    string query_string_ctl;

        
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
        }else if(strcmp(argv[i], "-ctl") == 0){
            isCTLlogic = true;
            if(i == argc-1){
                fprintf(stderr, "Missing ctl Algorithm after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            else{
                char *s = argv[++i];
                if(strcmp(s, "local") == 0){
                    ctl_algorithm = LOCAL;
                }
                else if(strcmp(s, "czero") == 0){
                    ctl_algorithm = CZERO;
                }
                else{
                    fprintf(stderr, "Argument Error: Unrecognized ctl algorithm \"%s\"\n", s);
                    return ErrorCode;
                }
            }
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
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--memory-limit") == 0) {
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
		}else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
				if (i == argc - 1) {
					fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
					return ErrorCode;
				}

				if (sscanf(argv[++i], "%d", &enablereduction) != 1 || enablereduction < 0 || enablereduction > 2) {
					fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
					return ErrorCode;
				}
		} else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0){
			printf(	"Usage: verifypn [options] model-file query-file\n"
					"A tool for answering CTL and reachability of place cardinality queries (including deadlock)\n" 
                                        "for weighted P/T Petri nets extended with inhibitor arcs.\n"
					"\n"
					"Options:\n"
					"  -k, --k-bound <number of tokens>   Token bound, 0 to ignore (default)\n"
					"  -t, --trace                        Provide XML-trace to stderr\n"
					"  -s, --search-strategy <strategy>   Search strategy:\n"
					"                                     - BestFS       Heuristic search (default)\n"
					"                                     - BFS          Breadth first search \n"
					"                                     - DFS          Depth first search (also works for CTL)\n"
					"                                     - RDFS         Random depth first search\n"
					"                                     - OverApprox   Linear Over Approx\n"
                                      
					"  -m, --memory-limit <megabyte>      Memory limit for the state space search in MB,\n"
					"                                     0 for unlimited (default)\n"
					"  -e, --state-space-exploration      State-space exploration only (query-file is irrelevant)\n"
					"  -x, --xml-query <query index>      Parse XML query file and verify query of a given index\n"
					"  -d, --disable-over-approximation   Disable linear over approximation\n"
                                        "  -r, --reduction                    Enable structural net reduction:\n"
                                        "                                     - 0  disabled (default)\n"
                                        "                                     - 1  aggressive reduction\n"
                                        "                                     - 2  reduction preserving k-boundedness\n"
					"  -n, --no-statistics                Do not display any statistics (default is to display it)\n"
					"  -h, --help                         Display this help message\n"
					"  -v, --version                      Display version information\n"
                                        "  -ctl <algorithm>                   CTL query verification in VerifyPN-CTL (algorithm must be specified):\n"
                                        "                                     - czero       Certain zero early termination algorithm \n"
                                        "                                     - local       Local algorithm\n"
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
			printf("Copyright (C) 2011-2016 Jonas Finnemann Jensen <jopsen@gmail.com>,\n");
			printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>,\n");
			printf("                        Lars Kærlund Østergaard <larsko@gmail.com>,\n");
			printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
			printf("CTL Engine (C) 2016	Isabella Kaufmann <ikaufm12@student.aau.dk>,\n");
 			printf("			Lasse Steen Jensen <lasjen12@student.aau.dk>,\n");
 			printf("			Søren Moss Nielsen <smni12@student.aau.dk>\n");
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
		searchstrategy = BFS;
	}
        if (isCTLlogic && enablereduction!=0) {
		// for CTL logic using reductions is unsafe		
                fprintf(stderr, "Structural reductions may not be used with CTL logic.\n\n");
                return ErrorCode;
        }
        if (isCTLlogic && searchstrategy == BestFS) {
		// the default search strategy for CTL logic is DFS
		searchstrategy = DFS;
	}
        if (isCTLlogic && searchstrategy != DFS) {
		// for CTL logic only DFS strategy is supported		
                fprintf(stderr, "Only DFS search strategy may be used with CTL logic.\n\n");
                return ErrorCode;
	}

	//----------------------- Validate Arguments -----------------------//
	//Check for model file
	if(!modelfile){
		fprintf(stderr, "Argument Error: No model-file provided\n");
		return ErrorCode;
	}

	//Check for query file
	if(!modelfile && !statespaceexploration){
		fprintf(stderr, "Argument Error: No query-file provided\n");
		return ErrorCode;
	}

	//----------------------- Open Model -----------------------//
        clock_t parse_model_begin = clock();
	//Load the model, begin scope to release memory from the stack
	PetriNet* net = NULL;
	MarkVal* m0 = NULL;
	VarVal* v0 = NULL;
                  
    // List of inhibitor arcs and transition enabledness
    PNMLParser::InhibitorArcList inhibarcs;
    PNMLParser::TransitionEnablednessMap transitionEnabledness;
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
		PetriNetBuilder builder(false);
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
    clock_t parse_model_end = clock();
    if(printstatistics) {
        cout<<":::TIME::: Parse model elapsed time: "<<double(diffclock(parse_model_end,parse_model_begin))<<" ms"<<endl;
    }
    //----------------------- Parse CTL Query -----------------------//
    clock_t parse_ctl_query_begin = clock();
    CTLFormula *queryList[15];
    
    if(isCTLlogic){
        std::string modelname;

        ifstream mfile(modelfile, ifstream::in);
        stringstream buf;
        buf << mfile.rdbuf();
        std::string str = buf.str();
        mfile.close();

        //Gets the name of the petrinet being passed
        for(auto iter = str.begin(); iter != str.end(); iter++){
            if((*iter) == 'i'){
                iter++;
                if((*iter) == 'd'){
                    iter++;
                    if((*iter) == '='){
                        iter++;
                        if((*iter) == '"'){
                            while(true){
                                iter++;
                                if((*iter) != '"'){
                                    modelname.push_back(*iter);
                                }
                                else
                                    break;
                            }
                            break;
                        }
                    }
                }
            }
        }

        mfile.close();
        if (printstatistics) {
        	std::cout << "Analysis:: Modefile: " << modelfile << endl;
        	std::cout << "Analysis:: Modelname: " << modelname << endl;
        	std::cout << "Analysis:: Number of places: " << net->numberOfPlaces() << endl;
        	std::cout << "Analysis:: Number of transistions: " << net->numberOfTransitions() << endl;
	}

        ifstream xmlfile (queryfile);
        vector<char> buffer((istreambuf_iterator<char>(xmlfile)), istreambuf_iterator<char>());
        buffer.push_back('\0');

        CTLParser ctlParser = CTLParser(net);
        
        ctlParser.ParseXMLQuery(buffer, queryList);
        
        clock_t parse_ctl_query_end = clock();
        if(printstatistics)
            cout<<":::TIME::: Parse of CTL query elapsed time: "<<double(diffclock(parse_ctl_query_end,parse_ctl_query_begin))<<" ms\n"<<endl;
    }
    
	//----------------------- Parse Reachability Query -----------------------//
        
	//Condition to check
	Condition* query = NULL;
	bool isInvariant = false;
	QueryXMLParser XMLparser(transitionEnabledness); // parser for XML queries
	//Read query file, begin scope to release memory
	if (!isCTLlogic) {
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
				querystring = querystr.substr(2);
				isInvariant = XMLparser.queries[xmlquery - 1].negateResult;
                
			} else { // standard textual query
				fprintf(stdout, "Query:  %s \n", querystr.c_str());
				//Validate query type
				if (querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG") {
					fprintf(stderr, "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n", querystr.substr(0, 2).c_str());
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
		} else { // state-space exploration or CTL
			querystring = "false";
			isInvariant = false;
		}
	
		//Parse query
		query = ParseQuery(querystring);
		if (printstatistics) {
                	cout<<":::::::::::::::::::\n"<<querystring<<endl;
		}
		if(!query){
			fprintf(stderr, "Error: Failed to parse query \"%s\"\n", querystring.c_str()); //querystr.substr(2).c_str());
			return ErrorCode;
		}
	}
        

	//----------------------- Context Analysis -----------------------//

	//Create scope for AnalysisContext
	if (!isCTLlogic){
		//Context analysis
		AnalysisContext context(*net);
		query->analyze(context);

		//Print errors if any
		if(context.errors().size() > 0){
			for(size_t i = 0; i < context.errors().size(); i++){
				fprintf(stderr, "Query Context Analysis Error: %s\n", context.errors()[i].toString().c_str());
			}
			return ErrorCode;
		}
	}

	
    //--------------------- Apply Net Reduction ---------------//
    Reducer reducer = Reducer(net); // reduced is needed also in trace generation (hence the extended scope)
	if ((enablereduction == 1 or enablereduction == 2) && !isCTLlogic) {
		// Compute how many times each place appears in the query
		MarkVal* placeInQuery = new MarkVal[net->numberOfPlaces()];
		for (size_t i = 0; i < net->numberOfPlaces(); i++) {
			placeInQuery[i] = 0;
		}
                QueryPlaceAnalysisContext placecontext(*net, placeInQuery);
                //Translate from CTL
                if(isCTLlogic){
                    string reductionquerystr;
                    reductionquerystr += "(";
                    bool firstAccurance = true;
                    int i = 0;
                    vector<string> AllQeuryPlaces;
                    CTLTree* current = queryList[xmlquery-1]->Query;
                    vector<string> *QueryPlaces = new vector<string>();
                    getQueryPlaces(QueryPlaces, current, net);
                    AllQeuryPlaces.reserve(AllQeuryPlaces.size() + QueryPlaces->size());
                    AllQeuryPlaces.insert(AllQeuryPlaces.end(), QueryPlaces->begin(), QueryPlaces->end());

                    vector<string> UniqueAllQeuryPlaces;
                    for(auto a : AllQeuryPlaces){
                        bool isUnique = true;
                        string newplace = "0 <= \"" + a + "\"";
                        if(UniqueAllQeuryPlaces.empty()){

                            UniqueAllQeuryPlaces.insert(UniqueAllQeuryPlaces.end(), newplace);
                            isUnique = false;
                        }
                        else {
                            for(auto u : UniqueAllQeuryPlaces){
                                if (newplace.compare(u) == 0){
                                    isUnique = false;
                                }
                            }
                        }
                        if (isUnique){
                            UniqueAllQeuryPlaces.insert(UniqueAllQeuryPlaces.end(), newplace);
                        }
                    }
                    for(auto a : UniqueAllQeuryPlaces){
                        if(!firstAccurance)
                            reductionquerystr += ") and (";
                        reductionquerystr += a;
                        firstAccurance = false;
                    }
                    reductionquerystr += ")";



                    Condition* reductionquery;

                    if(!firstAccurance) {
                        reductionquery = ParseQuery(reductionquerystr);
                        reductionquery->analyze(placecontext);
                    }
                }
                else {
                    query->analyze(placecontext);
                }
                
		// Compute the places and transitions that connect to inhibitor arcs
		MarkVal* placeInInhib = new MarkVal[net->numberOfPlaces()];
		MarkVal* transitionInInhib = new MarkVal[net->numberOfTransitions()];

		// CreateInhibitorPlacesAndTransitions translates inhibitor place/transitions names to indexes
		reducer.CreateInhibitorPlacesAndTransitions(net, inhibarcs, placeInInhib, transitionInInhib);

		//reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib); 
		reducer.Reduce(net, m0, placeInQuery, placeInInhib, transitionInInhib, enablereduction); // reduce the net
		//reducer.Print(net, m0, placeInQuery, placeInInhib, transitionInInhib);
	}
    
	//----------------------- Reachability -----------------------//

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
        else if(isCTLlogic){
                strategy = NULL;
                disableoverapprox = true;
        }
	else{
		fprintf(stderr, "Error: Search strategy selection out of range.\n");
		return ErrorCode;
	}

	// Wrap in linear over-approximation, if not disabled
	if(!disableoverapprox)
		strategy = new LinearOverApprox(strategy);

	// If no strategy is provided
	if(!strategy && !isCTLlogic){
		fprintf(stderr, "Error: No search strategy provided!\n");
		return ErrorCode;
	}
        
        
    ReturnValues retval = ErrorCode;
    if(!isCTLlogic){
//-------------------------------------------------------------------//
//----------------------- Reachability search -----------------------//
//-------------------------------------------------------------------//
	ReachabilityResult result = strategy->reachable(*net, m0, v0, query);

	
	//----------------------- Output Result -----------------------//
	
	const std::vector<std::string>& tnames = net->transitionNames();
	const std::vector<std::string>& pnames = net->placeNames();

	

	if (statespaceexploration) {
		retval = UnknownCode;
		unsigned int placeBound = 0;
		for(size_t p = 0; p < result.maxPlaceBound().size(); p++) { 
			placeBound = std::max<unsigned int>(placeBound,result.maxPlaceBound()[p]);
		}
		fprintf(stdout,"STATE_SPACE STATES %lli TECHNIQUES EXPLICIT\n", result.exploredStates());
        	fprintf(stdout,"STATE_SPACE TRANSITIONS -1 TECHNIQUES EXPLICIT\n");
        	fprintf(stdout,"STATE_SPACE MAX_TOKEN_PER_MARKING %d TECHNIQUES EXPLICIT\n", result.maxTokens());
        	fprintf(stdout,"STATE_SPACE MAX_TOKEN_IN_PLACE %d TECHNIQUES EXPLICIT\n", placeBound);               
        	return retval;
	}
	
	//Find result code
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
			fprintf(stdout, "FORMULA %s TRUE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", XMLparser.queries[xmlquery-1].id.c_str());
		}
        fprintf(stdout, "\nQuery is satisfied.\n\n");
	} else if(retval == FailedCode) {
		if (xmlquery>0 && XMLparser.queries[xmlquery-1].isPlaceBound) {
			// find index of the place for reporting place bound
			for(size_t p = 0; p < result.maxPlaceBound().size(); p++) { 
				if (pnames[p]==XMLparser.queries[xmlquery-1].placeNameForBound) {
					fprintf(stdout, "FORMULA %s %d TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", XMLparser.queries[xmlquery-1].id.c_str(), result.maxPlaceBound()[p]);
					fprintf(stdout, "\nMaximum number of tokens in place %s: %d\n\n",XMLparser.queries[xmlquery-1].placeNameForBound.c_str(),result.maxPlaceBound()[p]);
                    retval = UnknownCode;
                    			break;
				}
			}
		} else {
			if (xmlquery>0) {
				fprintf(stdout, "FORMULA %s FALSE TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT STRUCTURAL_REDUCTION\n", XMLparser.queries[xmlquery-1].id.c_str());
			}
            fprintf(stdout, "\nQuery is NOT satisfied.\n\n");
		}
	}
	
	//----------------------- Output Trace -----------------------//
	
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
        }
        
//-------------------------------------------------------------------//
//---------------------------- CTL search ---------------------------//
//-------------------------------------------------------------------//
        else {
            ReturnValues retval[16];

            clock_t total_search_begin = clock();
            search_ctl_query(net, m0, queryList, xmlquery, ctl_algorithm, searchstrategy, retval, inhibarcs);
            clock_t total_search_end = clock();

            if(printstatistics){
                cout<<"\n:::TIME::: Total search elapsed time: "<<double(diffclock(total_search_end,total_search_begin))<<" ms\n"<<endl;
            }
        }
//------------------------ Return the Output Value -------------------//
        return retval;
}

