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
//        cerr << "ERROR: DFS not implemented yet" << std::endl;
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
    start_index = start_index > 0 ? start_index : 0;
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

void verifypnCTL(PetriEngine::PetriNet *net,
                 PetriEngine::MarkVal *m0,
                 PNMLParser::InhibitorArcList &inhibitorarcs,
                 string modelname,
                 vector<CTLQuery *> &queries,
                 int xmlquery,
                 int algorithm,
                 int strategy,
                 bool print_statistics)
{
    //Initialization area
    std::vector<CTLResult> ctlresults = makeResults(modelname, queries, xmlquery, print_statistics);

    PetriNets::OnTheFlyDG graph = PetriNets::OnTheFlyDG(net, m0, inhibitorarcs);

    FixedPointAlgorithm *FPA = get<FixedPointAlgorithm>(algorithm);
    DistributedFixedPointAlgorithm *dFPA = get<DistributedFixedPointAlgorithm>(algorithm);

    Communicator *comm = nullptr;
    PartitionFunction *partition = nullptr;
    bool print = true;

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

            if(comm != nullptr){

            }
            else
                comm = new MPICommunicator(&graph);


            partition == nullptr ? partition = new HashPartitionFunction(comm->size()) : partition;

            print = comm->rank() == 0 ? true : false;

            assert(stg != nullptr && comm != nullptr && partition != nullptr);
            search(result, *dFPA, graph, *stg, *comm, *partition);
        }

        if(result.statistics_level > 0){
            cout << "::TIME:: " << result.duration << endl;
        }
        if(result.statistics_level > 1){
            //Add when supported
        }
        if(print)
            cout << "FORMULA " << result.modelname << "-" << result.query_nbr << " " << boolalpha << result.answer << endl;
    }

    //process answers

    //Finalize MPI
    if(comm != nullptr)
        delete comm;
}
