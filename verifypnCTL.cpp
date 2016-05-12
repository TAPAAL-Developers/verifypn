#include "verifypnCTL.h"

//Ctl includes
///Algorithms
#include "CTL/Algorithm/FixedPointAlgorithm.h"
#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"
#include "CTL/Algorithm/DistCZeroFPA.h"
#include "CTL/PetriNets/OnTheFlyDG.h"

///Strategies
#include "CTL/SearchStrategy/DFSSearch.h"
#include "CTL/SearchStrategy/BasicDistStrategy.h"
#include "CTL/SearchStrategy/BasicSearchStrategy.h"

#include <iostream>
#include <cstdlib>

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
    LOCAL   = 0,
    CZERO   = 1,
    DIST    = 2
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

template<class strategy_type>
strategy_type *get_strategy(int strategy){
    template_invokation_error(__func__, __LINE__);
    return nullptr;
}

template<>
iSequantialSearchStrategy *get_strategy<iSequantialSearchStrategy>(int strategy){
    if(strategy == DFS){
//        cerr << "ERROR: DFS not implemented yet" << std::endl;
        return new DFSSearch();
    }
    else if(strategy == BFS){
        cerr << "ERROR: BFS not implemented yet" << std::endl;
    }
    else {
        cerr << "ERROR: unsupported/unknown sequential search strategy." << std::endl;
        return nullptr;
    }
}

template<>
iDistributedSearchStrategy *get_strategy<iDistributedSearchStrategy>(int strategy){
    if(strategy == DFS){
        cerr << "ERROR: DFS not implemented yet" << std::endl;
    }
    else if(strategy == BFS){
        cerr << "ERROR: BFS not implemented yet" << std::endl;
    }
    else {
        cerr << "ERROR: unsupported/unknown distributed search strategy." << std::endl;
        return nullptr;
    }
}

FixedPointAlgorithm *get_algorithm(int algorithm){
    if(algorithm == LOCAL){
        return new LocalFPA();
    }
    else if(algorithm == CZERO){
        return new CertainZeroFPA();
    }
    else if(algorithm == DIST){
        return new DistCZeroFPA();
    }
    else {
        cerr << "ERROR: Unknown sequential fixed point algorithm" << std::endl;
        return nullptr;
    }
}

double verifypnCTL(PetriEngine::PetriNet *net,
                   PetriEngine::MarkVal *m0,
                   vector<CTLQuery *> &queries,
                   int xmlquery,
                   int algorithm,
                   int strategy,
                   vector<int> results,
                   PNMLParser::InhibitorArcList inhibitorarcs,
                   bool print_statistics)
{

    int currentQ = xmlquery > 0 ? xmlquery - 1 : 0;
    int endQ = xmlquery > 0 ? xmlquery : queries.size() - 1;

    PetriNets::OnTheFlyDG graph = PetriNets::OnTheFlyDG(net, m0, inhibitorarcs);

    double total_time = 0;
    stopwatch timer;




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
