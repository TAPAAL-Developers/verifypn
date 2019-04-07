#include "CTLEngine.h"

#include "DependencyGraph/OnTheFlyDG.h"
#include "CTLResult.h"
#include "Utils/DependencyGraph/Edge.h"

#include "Utils/DependencyGraph/SearchStrategy/DFSSearch.h"

#include "Utils/DependencyGraph/CertainZeroFPA.h"
#include "Utils/DependencyGraph/LocalFPA.h"

#include "PetriEngine/PQL/PQL.h"

#include "Utils/Stopwatch.h"
#include "PetriEngine/options.h"
#include "Utils/DependencyGraph/AlgorithmTypes.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <vector>

using namespace std;
using namespace PetriEngine::PQL;


ReturnValue getAlgorithm(std::shared_ptr<DependencyGraph::FixedPointAlgorithm>& algorithm,
                         DependencyGraph::AlgorithmType algorithmtype, Utils::SearchStrategies::Strategy search)
{
    switch(algorithmtype)
    {
        case DependencyGraph::Local:
            algorithm = std::make_shared<DependencyGraph::LocalFPA>(search);
            break;
        case DependencyGraph::CZero:
            algorithm = std::make_shared<DependencyGraph::CertainZeroFPA>(search);
            break;
        default:
            cerr << "Error: Unknown or unsupported algorithm" << endl;
            return ErrorCode;
    }
    return ContinueCode;
}

void printResult(const std::string& qname, CTLResult& result, bool statisticslevel, bool mccouput, bool only_stats, size_t index, options_t& options){
    const static string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STATE_COMPRESSION SAT_SMT ";

    if(!only_stats)
    {
        cout << endl;
        cout << "FORMULA "
             << qname
             << " " << (result.result ? "TRUE" : "FALSE") << " "
             << techniques
             << (options.isCPN ? "UNFOLDING_TO_PT " : "")
             << (options.stubbornreduction ? "STUBBORN_SETS " : "")
             << (options.ctlalgorithm == DependencyGraph::CZero ? "CTL_CZERO " : "")
             << (options.ctlalgorithm == DependencyGraph::Local ? "CTL_LOCAL " : "")
                << endl << endl;
        std::cout << "Query index " << index << " was solved" << std::endl;
        cout << "Query is" << (result.result ? "" : " NOT") << " satisfied." << endl;

        cout << endl;
    }
    if(statisticslevel){
        cout << "STATS:" << endl;
        cout << "	Time (seconds)    : " << setprecision(4) << result.duration / 1000 << endl;
        cout << "	Configurations    : " << result.numberOfConfigurations << endl;
        cout << "	Markings          : " << result.numberOfMarkings << endl;
        cout << "	Edges             : " << result.numberOfEdges << endl;
        cout << "	Processed Edges   : " << result.processedEdges << endl;
        cout << "	Processed N. Edges: " << result.processedNegationEdges << endl;
        cout << "	Explored Configs  : " << result.exploredConfigurations << endl;
        std::cout << endl;
    }
}

ReturnValue CTLMain(PetriEngine::PetriNet* net,
                    DependencyGraph::AlgorithmType algorithmtype,
                    Utils::SearchStrategies::Strategy strategytype,
                    bool gamemode,
                    bool printstatistics,
                    bool mccoutput,
                    bool partial_order,
                    const std::vector<std::string>& querynames,
                    const std::vector<std::shared_ptr<Condition>>& queries,
                    const std::vector<size_t>& querynumbers,
                    options_t& options
        )
{

    for(auto qnum : querynumbers){
        CTLResult result(queries[qnum]);
        PetriNets::OnTheFlyDG graph(net, partial_order); 
        graph.setQuery(result.query);
        std::shared_ptr<DependencyGraph::FixedPointAlgorithm> alg = nullptr;
        bool solved = false;
        switch(graph.initialEval())
        {
            case Condition::Result::RFALSE:
                result.result = false;
                solved = true;
                break;
            case Condition::Result::RTRUE:
                result.result = true;
                solved = true;
                break;
            default:
                break;
        }
        
        if(!solved)
        {            
            if(getAlgorithm(alg, algorithmtype,  strategytype) == ErrorCode)
            {
                return ErrorCode;
            }

            stopwatch timer;
            timer.start();
            result.result = alg->search(graph);
            timer.stop();

            result.duration = timer.duration();
        }
        result.numberOfConfigurations = graph.configurationCount();
        result.numberOfMarkings = graph.markingCount();
        result.processedEdges = alg ? alg->processedEdges() : 0;
        result.processedNegationEdges = alg ? alg->processedNegationEdges() : 0;
        result.exploredConfigurations = alg ? alg->exploredConfigurations() : 0;
        result.numberOfEdges = alg ? alg->numberOfEdges() : 0;
        printResult(querynames[qnum], result, printstatistics, mccoutput, false, qnum, options);
    }
    return SuccessCode;
}
