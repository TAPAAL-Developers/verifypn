#include "verifypnCTL.h"

//Ctl includes
///Algorithms
#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/Algorithm/DistCZeroFPA.h"

///Strategies
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/UniversalSearchStrategy.h"

///Strategy dependencies
#include "CTL/SearchStrategy/WaitingList.h"
#include "CTL/SearchStrategy/NegationWaitingList.h"

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
        cerr << "ERROR: BFS not implemented yet" << std::endl;
    }
    else {
//        cerr << "ERROR: Unknown distributed search strategy." << std::endl;
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

std::vector<CTLResult> &&makeResults(std::string &modelname,
                                     std::vector<CTLQuery*> queries,
                                     int statistics_level)
{
    std::vector<CTLResult> ctlresults;
    ctlresults.reserve(queries.size());

    for(int i; i < queries.size(); i++){
        ctlresults[i] = CTLResult(modelname, queries[i], i, 1);
    }
}

template<class Algorithm, class... Args>
bool search(CTLResult &result, Algorithm &algorithm, Args&&... args){
    stopwatch timer;
    timer.start();

    bool answer = algorithm->search(args...);

    timer.stop();
    result.duration = timer.duration();

    return answer;
}

void verifypnCTL(PetriEngine::PetriNet *net,
                 PetriEngine::MarkVal *m0,
                 string modelname,
                 vector<CTLQuery *> &queries,
                 int xmlquery,
                 int algorithm,
                 int strategy,
                 PNMLParser::InhibitorArcList inhibitorarcs,
                 bool print_statistics)
{
    std::vector<CTLResult> ctlresults = makeResults(modelname, queries, RUNNINGTIME);

    int firstQ = xmlquery > 0 ? xmlquery - 1 : 0;
    int lastQ = xmlquery > 0 ? xmlquery : queries.size();

    PetriNets::OnTheFlyDG graph = PetriNets::OnTheFlyDG(net, m0, inhibitorarcs);

    FixedPointAlgorithm *FPA = get<FixedPointAlgorithm>(algorithm);
    DistributedFixedPointAlgorithm *dFPA = get<DistributedFixedPointAlgorithm>(algorithm);

    //Begin:    Sequential call area
    if(algorithm != DIST)
    {
        while(firstQ < lastQ){
            CTLResult &result = ctlresults[firstQ++];
            graph.setQuery(result.query);

            auto *stg = get<iSequantialSearchStrategy>(strategy);
            bool answer = search(result, FPA, graph, *stg);

            if(firstQ < lastQ){
                graph.cleanUp();
            }
        }
    }
    //End:      Sequential call area

    //Begin:    Distributed call area
    else {

    }
    //End:      Distributed call area


//    if(t_xmlquery > 0){
//        clock_t individual_search_begin = clock();

//        graph->setQuery(queryList.front());
//        res = algorithm->search(*graph, *strategy);

//        std::cout << std::boolalpha << "answer is " << res << std::endl;

//        configCount = 0;//graph->configuration_count();
//        markingCount = 0;//graph->marking_count();
//        //queryList[t_xmlquery - 1]->Result = res;

//        clock_t individual_search_end = clock();
//        if (printstatistics) {
//            cout<<":::TIME::: Search elapsed time for query "<< t_xmlquery - 1 <<": "<<double(diffclock(individual_search_end,individual_search_begin))<<" ms"<<endl;
//            cout<<":::DATA::: Configurations: " << configCount << " Markings: " << markingCount << endl;
//        }
//        if (res){
//            results[t_xmlquery - 1] = SuccessCode;
//        }
//        else if (!res){
//            results[t_xmlquery - 1] = FailedCode;
//        }
//        else results[t_xmlquery - 1] = ErrorCode;

//        //queryList[t_xmlquery - 1]->pResult();
//    }
}
