#include "CTL/CTLEngine.h"

#include "CTL/PetriNets/OnTheFlyDG.h"
#include "CTL/CTLResult.h"


#include "CTL/Algorithm/CertainZeroFPA.h"
#include "CTL/Algorithm/LocalFPA.h"


#include "CTL/Stopwatch.h"
#include "options.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/TAR/TARReachability.h"

#include <iostream>
#include <iomanip>
#include <vector>
#include <PetriEngine/PQL/Expressions.h>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;
using namespace PetriNets;

namespace CTL {
error_e get_algorithm(std::shared_ptr<Algorithm::FixedPointAlgorithm>& algorithm,
    CTLAlgorithmType algorithmtype, options_t::search_strategy_e search)
{
    switch(algorithmtype)
    {
        case CTLAlgorithmType::Local:
            algorithm = std::make_shared<Algorithm::LocalFPA>(search);
            break;
        case CTLAlgorithmType::CZero:
            algorithm = std::make_shared<Algorithm::CertainZeroFPA>(search);
            break;
        default:
            throw base_error("Error: Unknown or unsupported algorithm");
    }
    return ContinueCode;
}

void print_ctl_result(const std::string& qname, const CTLResult& result, size_t index, options_t& options) {
    const static string techniques = "TECHNIQUES COLLATERAL_PROCESSING EXPLICIT STATE_COMPRESSION SAT_SMT ";

    cout << endl;
    cout << "FORMULA "
         << qname
        << " " << (result._result ? "TRUE" : "FALSE") << " "
         << techniques
         << (options._is_CPN ? "UNFOLDING_TO_PT " : "")
         << (options._stubborn_reduction ? "STUBBORN_SETS " : "")
         << (options._ctlalgorithm == CTL::CZero ? "CTL_CZERO " : "")
         << (options._ctlalgorithm == CTL::Local ? "CTL_LOCAL " : "")
            << endl << endl;
    std::cout << "Query index " << index << " was solved" << std::endl;
    cout << "Query is" << (result._result ? "" : " NOT") << " satisfied." << endl;

    cout << endl;

    if(options._print_statistics){
        cout << "STATS:" << endl;
        cout << "	Time (seconds)    : " << setprecision(4) << result._duration / 1000 << endl;
        cout << "	Configurations    : " << result._numberOfConfigurations << endl;
        cout << "	Markings          : " << result._numberOfMarkings << endl;
        cout << "	Edges             : " << result._numberOfEdges << endl;
        cout << "	Processed Edges   : " << result._processedEdges << endl;
        cout << "	Processed N. Edges: " << result._processedNegationEdges << endl;
        cout << "	Explored Configs  : " << result._exploredConfigurations << endl;
        std::cout << endl;
    }
}

bool single_solve(const Condition_ptr& query, const PetriNet& net, CTLResult& result, options_t& options)
{
    OnTheFlyDG graph(net, options._stubborn_reduction);
    graph.set_query(query);
    std::shared_ptr<Algorithm::FixedPointAlgorithm> alg = nullptr;
    if(get_algorithm(alg, options._ctlalgorithm,  options._strategy) == ErrorCode)
    {
        assert(false);
        throw std::exception();
    }

    stopwatch timer;
    timer.start();
    auto res = alg->search(graph);
    timer.stop();

    result._duration += timer.duration();
    result._numberOfConfigurations += graph.configuration_count();
    result._numberOfMarkings += graph.marking_count();
    result._processedEdges += alg->processed_edges();
    result._processedNegationEdges += alg->processed_negation_edges();
    result._exploredConfigurations += alg->explored_configurations();
    result._numberOfEdges += alg->number_of_edges();
    return res;
}

bool recursive_solve(const Condition_ptr& query, const PetriNet& net, CTLResult& result, options_t& options);

class ResultHandler : public AbstractHandler {
    private:
        bool _is_conj = false;
        const std::vector<int8_t>& _lstate;
    public:
        ResultHandler(bool is_conj, const std::vector<int8_t>& lstate)
        : _is_conj(is_conj), _lstate(lstate)
        {}

        std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                const PQL::Condition& query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                const Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking) const override
        {
            if(result == ResultPrinter::Satisfied)
            {
                result = _lstate[index] < 0 ? ResultPrinter::NotSatisfied : ResultPrinter::Satisfied;
            }
            else if(result == ResultPrinter::NotSatisfied)
            {
                result = _lstate[index] < 0 ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
            }
            bool terminate = _is_conj ? (result == ResultPrinter::NotSatisfied) : (result == ResultPrinter::Satisfied);
            return std::make_pair(result, terminate);
        }
};

bool solve_logical_condition(LogicalCondition* query, bool is_conj, const PetriNet& net,
                           CTLResult& result, options_t& options)
{
    std::vector<int8_t> state(query->size(), 0);
    std::vector<int8_t> lstate;
    std::vector<Condition_ptr> queries;
    for(size_t i = 0; i < query->size(); ++i)
    {
        if((*query)[i]->is_reachability())
        {
            state[i] = dynamic_cast<NotCondition*>((*query)[i].get()) ? -1 : 1;
            queries.emplace_back((*query)[i]->prepare_for_reachability());
            lstate.emplace_back(state[i]);
        }
    }

    {
        ResultHandler handler(is_conj, lstate);
        std::vector<AbstractHandler::Result> res(queries.size(), AbstractHandler::Unknown);
        if(!options._tar)
        {
            ReachabilitySearch strategy(net, handler, options._kbound, true);
            strategy.reachable(queries, res,
                                        options._strategy,
                                        options._stubborn_reduction,
                                        false,
                                        false,
                                        false,
                                        options.seed());
        }
        else
        {
            TARReachabilitySearch tar(handler, net, nullptr, options._kbound);
            tar.reachable(queries, res, false, false);
        }
        size_t j = 0;
        for(size_t i = 0; i < query->size(); ++i) {
            if (state[i] != 0)
            {
                if (res[j] == AbstractHandler::Unknown) {
                    ++j;
                    continue;
                }
                auto bres = res[j] == AbstractHandler::Satisfied;

                if(bres xor is_conj) {
                    return !is_conj;
                }
                ++j;
            }
        }
    }

    for(size_t i = 0; i < query->size(); ++i) {
        if (state[i] == 0)
        {
            if(recursive_solve((*query)[i], net, result, options) xor is_conj)
            {
                return !is_conj;
            }
        }
    }
    return is_conj;
}

class SimpleResultHandler : public AbstractHandler
{
public:
    std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                const PQL::Condition& query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                const Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking) const {
        return std::make_pair(result, false);
    }
};

bool recursive_solve(const Condition_ptr& query, const PetriEngine::PetriNet& net,
    CTLResult& result, options_t& options)
{
    if(auto q = dynamic_cast<NotCondition*>(query.get()))
    {
        return ! recursive_solve((*q)[0], net, result, options);
    }
    else if(auto q = dynamic_cast<AndCondition*>(query.get()))
    {
        return solve_logical_condition(q, true, net, result, options);
    }
    else if(auto q = dynamic_cast<OrCondition*>(query.get()))
    {
        return solve_logical_condition(q, false, net, result, options);
    }
    else if(query->is_reachability())
    {
        SimpleResultHandler handler;
        std::vector<Condition_ptr> queries{query->prepare_for_reachability()};
        std::vector<AbstractHandler::Result> res;
        res.emplace_back(AbstractHandler::Unknown);
        if(options._tar)
        {
            TARReachabilitySearch tar(handler, net, nullptr, options._kbound);
            tar.reachable(queries, res, false, false);
        }
        else
        {
            ReachabilitySearch strategy(net, handler, options._kbound, true);
            strategy.reachable(queries, res,
                           options._strategy,
                           options._stubborn_reduction,
                           false,
                           false,
                           false,
                           options.seed());
        }
        return (res.back() == AbstractHandler::Satisfied) xor query->is_invariant();
    }
    else
    {
        return single_solve(query, net, result, options);
    }
}


CTLResult verify_ctl(const PetriNet& net,
        Condition_ptr& query,
        options_t& options
    )
{
    CTLResult result(query);
    bool solved = false;

    {
        OnTheFlyDG graph(net, options._stubborn_reduction);
        graph.set_query(result._query);
        switch (graph.initial_eval()) {
            case Condition::Result::RFALSE:
                result._result = false;
                solved = true;
                break;
            case Condition::Result::RTRUE:
                result._result = true;
                solved = true;
                break;
            default:
                break;
        }
    }
    result._numberOfConfigurations = 0;
    result._numberOfMarkings = 0;
    result._processedEdges = 0;
    result._processedNegationEdges = 0;
    result._exploredConfigurations = 0;
    result._numberOfEdges = 0;
    result._duration = 0;
    if(!solved)
    {
        result._result = recursive_solve(result._query, net, result, options);
    }
    return result;
}
}

