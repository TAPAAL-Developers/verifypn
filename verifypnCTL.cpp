#include "verifypnCTL.h"

//Ctl includes
///Algorithms
#include "CTL/Algorithm/FixedPointAlgorithm.h" //interface
#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/Algorithm/DistCZeroFPA.h"

///Strategies
#include "CTL/SearchStrategy/iSearchStrategy.h" //interface
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/UniversalSearchStrategy.h"

///Strategy dependencies
#include "CTL/SearchStrategy/WaitingList.h"
#include "CTL/SearchStrategy/NegationWaitingList.h"

///Commincators
#include "CTL/Communicator/Communicator.h" //interface
#include "CTL/Communicator/MPICommunicator.h"

///Partioning
#include "CTL/Algorithm/PartitionFunction.h"
#include "CTL/PetriNets/HashPartitionFunction.h"

///Graphs
#include "CTL/PetriNets/OnTheFlyDG.h"

//Std includes
#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <sys/resource.h>
#define MEM_ENV_VAR "MAXMEM_KB"

/* Enumeration of return values from VerifyPN */
enum ReturnValues{
    SuccessCode	= 0,
    FailedCode	= 1,
    UnknownCode	= 2,
    ErrorCode	= 3
};

/* Enumeration of search-strategies in VerifyPN */
///The extra unsupported searches is for compatibility with main()
enum SearchStrategies{
    BestFS,			//LinearOverAprox + UltimateSearch
    BFS,			//LinearOverAprox + BreadthFirstReachabilitySearch
    DFS,			//LinearOverAprox + DepthFirstReachabilitySearch
    RDFS,			//LinearOverAprox + RandomDFS
    OverApprox,		//LinearOverApprx
};

/* Enumeration of algorithms in verifypnCTL */
enum CtlAlgorithm {
    LOCAL   = 0,
    CZERO   = 1,
    DIST    = 2
};

enum CommunicatorType {
    MPI_COMM    = 0
};

enum Partioning {
    HASH    = 0
};

enum CtlStatisticsLevel{
    NONE = 0,
    RUNNINGTIME = 1,
    EVERYTHING = 2
};

using namespace SearchStrategy;
using namespace Algorithm;
using namespace PetriNets;

//helper function that is going to set a strict memory limit
void setmemlimit()
{
    struct rlimit memlimit;
    long bytes;

    if(getenv(MEM_ENV_VAR)!=NULL)
    {
        std::cout << "[Memory limit] " << (char*)getenv(MEM_ENV_VAR) << std::endl;
        bytes = atol(getenv(MEM_ENV_VAR))*1024;
        memlimit.rlim_cur = bytes;
        memlimit.rlim_max = bytes;
        setrlimit(RLIMIT_AS, &memlimit);
    }
    else {
        /*
        std::cout << "No memory limit set. Defaulting to 16000000" << endl;
        bytes = atol("16000000")*1024;
        memlimit.rlim_cur = bytes;
        memlimit.rlim_max = bytes;
        setrlimit(RLIMIT_AS, &memlimit);
        */
    }
}

//Invoking this template is cause for error
template<class func, class line_nbr>
void template_invokation_error(func f, line_nbr nbr){
    cerr << "Error: Invokation of base function template "
         << f << " " << nbr
         << " - This should not happen." << endl;
}

template<class T>
T *get(int type){
    template_invokation_error(__func__, __LINE__);
    return nullptr;
}

template<>
iSequantialSearchStrategy *get<iSequantialSearchStrategy>(int type){
    if(type == DFS){
        return new DFSSearch();
    }
    else if(type == BFS){
        return new UniversalSearchStrategy<WaitingList<Edge*, std::queue<Edge*>>>();
    }
    else {
//        cerr << "ERROR: Unknown sequential search strategy." << std::endl;
        return nullptr;
    }
}

template<>
iDistributedSearchStrategy *get<iDistributedSearchStrategy>(int type){
    if(type == DFS){
        return new UniversalSearchStrategy<>();
    }
    else if(type == BFS){
        return new UniversalSearchStrategy<WaitingList<Edge*, std::queue<Edge*>>>();
    }
    else {
//        cerr << "ERROR: Unknown distributed search strategy." << std::endl;
        return nullptr;
    }
}

template<>
DistributedFixedPointAlgorithm *get<DistributedFixedPointAlgorithm>(int type){
    if(type == DIST){
        return new DistCZeroFPA();
    }
    else {
//        cerr << "ERROR: Unknown sequential fixed point algorithm" << std::endl;
        return nullptr;
    }
}

template<>
FixedPointAlgorithm *get<FixedPointAlgorithm>(int type){
    if(type == LOCAL){
        return new LocalFPA();
    }
    else if(type == CZERO){
        return new CertainZeroFPA();
    }
    else {
//        cerr << "ERROR: Unknown sequential fixed point algorithm" << std::endl;
        return nullptr;
    }
}

std::vector<CTLResult> makeResults(std::string &modelname,
                                   std::vector<CTLQuery*> &queries,
                                   int start_index,
                                   int statistics_level)
{
    start_index = start_index > 0 ? start_index - 1 : 0;
    std::vector<CTLResult> ctlresults;

    auto qIter = queries.begin();
    for(int i = 0; i < queries.size(); i++){
        assert(qIter != queries.end());
        ctlresults.emplace_back(modelname, *qIter, start_index + i, statistics_level);
        qIter++;
    }

    return ctlresults;
}

template<class Algorithm, class Graph, class... Args>
void search(CTLResult &result,
            Algorithm &algorithm,
            Graph &graph,
            Args&&... args){
    stopwatch timer;
    timer.start();
    result.answer = algorithm.search(graph, args...);
    timer.stop();

    result.result = result.answer ? SuccessCode : FailedCode;

    result.duration = timer.duration();
}

int verifypnCTL(PetriEngine::PetriNet *net,
                 PetriEngine::MarkVal *m0,
                 PNMLParser::InhibitorArcList &inhibitorarcs,
                 string modelname,
                 vector<CTLQuery *> &queries,
                 int xmlquery,
                 int algorithm,
                 int strategy,
                 bool print_statistics)
{
    //set memory limit (if env. variable is present)
    setmemlimit();
    //Initialization area
    std::vector<CTLResult> ctlresults = makeResults(modelname, queries, xmlquery, print_statistics);

    PetriNets::OnTheFlyDG graph = *(new PetriNets::OnTheFlyDG(net, m0, inhibitorarcs));

    FixedPointAlgorithm *FPA = get<FixedPointAlgorithm>(algorithm);
    DistributedFixedPointAlgorithm *dFPA = get<DistributedFixedPointAlgorithm>(algorithm);

    Communicator *comm = nullptr;
    PartitionFunction *partition = nullptr;
    bool print = true;

    std::string seq_techniques("TECHNIQUES SEQUENTIAL_PROCESSING EXPLICIT");
    std::string dist_techniques("TECHNIQUES PARALLEL_PROCESSING EXPLICIT");

    //Main computation loop.
    for(CTLResult result : ctlresults){
        graph.setQuery(result.query);

        if(FPA != nullptr){
            iSequantialSearchStrategy *stg = get<iSequantialSearchStrategy>(strategy);
            assert(stg != nullptr);
            search(result, *FPA, graph, *stg);
        }
        else if(dFPA != nullptr){
            iDistributedSearchStrategy *stg = get<iDistributedSearchStrategy>(strategy);

            if(comm == nullptr) {
                comm = new MPICommunicator(&graph);
            } else {
                comm->reset();
            }


            partition == nullptr ? partition = new HashPartitionFunction(comm->size()) : partition;

            print = comm->rank() == 0 ? true : false;

            assert(stg != nullptr && comm != nullptr && partition != nullptr);
            search(result, *dFPA, graph, *stg, *comm, *partition);
        }

        if(result.statistics_level > 0){
            cout << "[Total Evaluation Time] " << result.duration << endl;
            //cout << "::TIME:: " << result.duration << endl;
        }
        if(result.statistics_level > 1){
            cout << "[No. Configurations] " << graph.configurationCount() << endl;
            cout << "[No. Markings] " << graph.markingCount() << endl;
        }
        if(print) {
            cout << "[Formula] " << result.modelname << "-" << result.query_nbr << endl;
            CTLParser_v2 p;
            cout << "[Formula Print] " << p.QueryToString(result.query) << endl;
            cout << "[Query Result] " << (result.answer ? "TRUE" : "FALSE") << endl;
        }
            /*cout << "FORMULA "
                 << result.modelname << "-"
                 << result.query_nbr << " "
                 << (result.answer ? "TRUE" : "FALSE") << " "
                 << (FPA != nullptr ? seq_techniques : dist_techniques)
                 << endl;*/
    }

    //process answers

    //Finalize MPI
    if(comm != nullptr)
        delete comm;

    return SuccessCode;
}
