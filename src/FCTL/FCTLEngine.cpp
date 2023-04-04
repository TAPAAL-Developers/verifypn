#include "FCTL/FCTLEngine.h"

#include "FCTL/PetriNets/FOnTheFlyDG.h"
#include "FCTL/FCTLResult.h"

#include "FCTL/Algorithm/FCertainZeroFPA.h"

#include "utils/stopwatch.h"
#include "PetriEngine/options.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/TAR/TARReachability.h"

#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PrepareForReachability.h"
#include "PetriEngine/PQL/PredicateCheckers.h"
#include "LTL/LTLSearch.h"

#include <iostream>
#include <iomanip>
#include <vector>

namespace Featured {
    using CTL::CTLAlgorithmType;
    using namespace PetriEngine;
    using namespace PetriEngine::PQL;
    using namespace PetriEngine::Reachability;
    using namespace PetriNets;

    ReturnValue getAlgorithm(std::shared_ptr<Algorithm::FixedPointAlgorithm>& algorithm,
                             CTLAlgorithmType algorithmtype, Strategy search) {
        switch (algorithmtype) {
            case CTLAlgorithmType::Local:
                throw base_error{"Do not want to use local algorithm in FCTL"};
                break;
            case CTLAlgorithmType::CZero:
                algorithm = std::make_shared<Algorithm::FCertainZeroFPA>(search);
                break;
            default:
                throw base_error("Unknown or unsupported algorithm");
        }
        return ReturnValue::ContinueCode;
    }

    std::pair<bdd, bdd> FCTLSingleSolve(const Condition_ptr& query, PetriNet* net,
                         CTLAlgorithmType algorithmtype,
                         Strategy strategytype, bool partial_order, FCTLResult& result) {
        return FCTLSingleSolve(query.get(), net, algorithmtype, strategytype, partial_order, result);
    }

    std::pair<bdd, bdd> FCTLSingleSolve(Condition* query, PetriNet* net,
                         CTLAlgorithmType algorithmtype,
                         Strategy strategytype, bool partial_order, bool negate, FCTLResult& result) {
        FOnTheFlyDG graph(net, partial_order);
        graph.setQuery(query);
        std::shared_ptr<Algorithm::FixedPointAlgorithm> alg = nullptr;
        getAlgorithm(alg, algorithmtype, strategytype);

        stopwatch timer;
        timer.start();
        auto res = alg->search(graph, negate);
        timer.stop();

        result.duration += timer.duration();
        result.numberOfConfigurations += graph.configurationCount();
        result.numberOfMarkings += graph.markingCount();
        result.processedEdges += alg->processedEdges();
        result.processedNegationEdges += alg->processedNegationEdges();
        result.exploredConfigurations += alg->exploredConfigurations();
        result.numberOfEdges += alg->numberOfEdges();
        result.maxTokens = std::max(graph.maxTokens(), result.maxTokens);
        return res;
    }

    std::pair<bdd, bdd> recursiveSolve(const Condition_ptr& query, PetriNet* net,
                        CTLAlgorithmType algorithmtype,
                        Strategy strategytype, bool partial_order, FCTLResult& result, bool negate, options_t& options);

    class ResultHandler : public AbstractHandler {
    private:
        bool _is_conj = false;
        const std::vector<int8_t>& _lstate;
    public:
        ResultHandler(bool is_conj, const std::vector<int8_t>& lstate)
                : _is_conj(is_conj), _lstate(lstate) {}

        std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking,
                bool) override {
            if (result == ResultPrinter::Satisfied) {
                result = _lstate[index] < 0 ? ResultPrinter::NotSatisfied : ResultPrinter::Satisfied;
            } else if (result == ResultPrinter::NotSatisfied) {
                result = _lstate[index] < 0 ? ResultPrinter::Satisfied : ResultPrinter::NotSatisfied;
            }
            bool terminate = _is_conj ? (result == ResultPrinter::NotSatisfied) : (result == ResultPrinter::Satisfied);
            return std::make_pair(result, terminate);
        }
    };

    std::pair<bdd, bdd> solveLogicalCondition(LogicalCondition* query, bool is_conj, PetriNet* net,
                               CTLAlgorithmType algorithmtype,
                               Strategy strategytype, bool partial_order, FCTLResult& result, bool negate, options_t& options) {
        std::vector<int8_t> state(query->size(), 0);
        std::vector<int8_t> lstate;
        std::vector<Condition_ptr> queries;
        /*for (size_t i = 0; i < query->size(); ++i) {
            if (PetriEngine::PQL::isReachability((*query)[i])) {
                state[i] = dynamic_cast<NotCondition*>((*query)[i].get()) ? -1 : 1;
                queries.emplace_back(prepareForReachability((*query)[i]));
                lstate.emplace_back(state[i]);
            }
        }*/

        /*{
            ResultHandler handler(is_conj, lstate);
            std::vector<AbstractHandler::Result> res(queries.size(), AbstractHandler::Unknown);
            if (!options.tar) {
                ReachabilitySearch strategy(*net, handler, options.kbound, true);
                strategy.reachable(queries, res,
                                   options.strategy,
                                   options.stubbornreduction,
                                   false,
                                   options.printstatistics,
                                   false,
                                   options.seed());
                result.maxTokens = std::max(result.maxTokens, strategy.maxTokens());
            } else {
                TARReachabilitySearch tar(handler, *net, nullptr, options.kbound);
                tar.reachable(queries, res, options.printstatistics, false);
            }
            size_t j = 0;
            for (size_t i = 0; i < query->size(); ++i) {
                if (state[i] != 0) {
                    if (res[j] == AbstractHandler::Unknown) {
                        ++j;
                        continue;
                    }
                    auto bres = res[j] == AbstractHandler::Satisfied;

                    if (bres xor is_conj) {
                        return !is_conj;
                    }
                    ++j;
                }
            }
        }*/

        bdd acc_good = is_conj ? (bdd)bddtrue : (bdd)bddfalse;
        bdd acc_bad =  is_conj ? (bdd)bddfalse : (bdd)bddtrue;
        for (size_t i = 0; i < query->size(); ++i) {
            if (state[i] == 0) {
                auto [good, bad] = recursiveSolve((*query)[i], net, algorithmtype, strategytype, partial_order, result, negate, options);
                if (is_conj) {
                    acc_good &= good;
                    acc_bad |= bad;
                    if (acc_good == bddfalse) {
                        return std::make_pair(bddfalse, bddtrue);
                    }
                }
                else {
                    acc_good |= good;
                    acc_bad &= bad;
                    if (acc_good == bddtrue) {
                        return std::make_pair(bddtrue, bddfalse);
                    }
                }
            }
        }
        return std::make_pair(acc_good, acc_bad);
    }

    class SimpleResultHandler : public AbstractHandler {
    public:
        std::pair<AbstractHandler::Result, bool> handle(
                size_t index,
                PQL::Condition* query,
                AbstractHandler::Result result,
                const std::vector<uint32_t>* maxPlaceBound,
                size_t expandedStates,
                size_t exploredStates,
                size_t discoveredStates,
                int maxTokens,
                Structures::StateSetInterface* stateset, size_t lastmarking, const MarkVal* initialMarking, bool) {
            return std::make_pair(result, false);
        }
    };

    std::pair<bdd, bdd> recursiveSolve(Condition* query, PetriEngine::PetriNet* net,
                        CTLAlgorithmType algorithmtype,
                        Strategy strategytype, bool partial_order, FCTLResult& result, bool negate, options_t& options);

    std::pair<bdd, bdd> recursiveSolve(const Condition_ptr& query, PetriEngine::PetriNet* net,
                        CTLAlgorithmType algorithmtype,
                        Strategy strategytype, bool partial_order, FCTLResult& result, bool negate, options_t& options) {
        return recursiveSolve(query.get(), net, algorithmtype, strategytype, partial_order, result, negate, options);
    }

    std::pair<bdd, bdd> recursiveSolve(Condition* query, PetriEngine::PetriNet* net,
                        CTLAlgorithmType algorithmtype,
                        Strategy strategytype, bool partial_order, FCTLResult& result, bool negate, options_t& options) {
        if (auto q = dynamic_cast<NotCondition*>(query)) {
            auto [good, bad] = recursiveSolve((*q)[0], net, algorithmtype, strategytype, partial_order, result, !negate, options);
            return std::make_pair(bad, good);
        } else if (auto q = dynamic_cast<AndCondition*>(query)) {
            return solveLogicalCondition(q, true, net, algorithmtype, strategytype, partial_order, result, negate, options);
        } else if (auto q = dynamic_cast<OrCondition*>(query)) {
            return solveLogicalCondition(q, false, net, algorithmtype, strategytype, partial_order, result, negate, options);
        } /*else if (false && !net->is_featured() && PetriEngine::PQL::isReachability(query)) {
            SimpleResultHandler handler;
            std::vector<Condition_ptr> queries{prepareForReachability(query)};
            std::vector<AbstractHandler::Result> res;
            res.emplace_back(AbstractHandler::Unknown);
            if (options.tar) {
                TARReachabilitySearch tar(handler, *net, nullptr, options.kbound);
                tar.reachable(queries, res, options.printstatistics, false);
            } else {
                ReachabilitySearch strategy(*net, handler, options.kbound, true);
                strategy.reachable(queries, res,
                                   options.strategy,
                                   options.stubbornreduction,
                                   false,
                                   options.printstatistics,
                                   false,
                                   options.seed());
                result.maxTokens = std::max(strategy.maxTokens(), result.maxTokens);
            }
            return (res.back() == AbstractHandler::Satisfied) xor query->isInvariant();
        }*/ else if (!containsNext(query)) {
            // there are probably many more cases w. nested quantifiers we can do
            // one instance is E[ non_temp U [E non_temp U ...]] in a chain
            // also, this should go into some *neat* visitor to do the check.
            auto q = query->shared_from_this();
            bool ok = false;
            if (auto* af = dynamic_cast<AFCondition*>(query)) {
                if (!isTemporal((*af)[0]))
                    ok = true;
            } else if (auto* eg = dynamic_cast<EGCondition*>(query)) {
                if (!isTemporal((*eg)[0]))
                    ok = true;
            } else if (auto* au = dynamic_cast<AUCondition*>(query)) {
                if (!isTemporal((*au)[0]) && !isTemporal((*au)[1]))
                    ok = true;
            } else if (auto* eu = dynamic_cast<EUCondition*>(query)) {
                if (!isTemporal((*eu)[0]) && !isTemporal((*eu)[1]))
                    ok = true;
            }
            /*if (false &&  ok && !net->is_featured()) {
                LTL::LTLSearch search(*net, q, options.buchiOptimization, options.ltl_compress_aps);
                auto r = search.solve(false, options.kbound, options.ltlalgorithm, options.ltl_por,
                                      options.strategy, options.ltlHeuristic, options.ltluseweak, options.seed_offset);

                result.maxTokens = std::max(search.max_tokens(), result.maxTokens);
                return r;
            }*/
        }
        //else
        {
            return FCTLSingleSolve(query, net, algorithmtype, strategytype, partial_order, negate, result);
        }
    }


    ReturnValue FCTLMain(PetriNet* net,
                         CTLAlgorithmType algorithmtype,
                         Strategy strategytype,
                         StatisticsLevel printstatistics,
                         bool partial_order,
                         const std::vector<std::string>& querynames,
                         const std::vector<std::shared_ptr<Condition>>& queries,
                         const std::vector<size_t>& querynumbers,
                         options_t& options
    ) {
        for (auto qnum: querynumbers) {
            FCTLResult result(queries[qnum]);
            bool solved = false;

            {
                FOnTheFlyDG graph(net, partial_order);
                graph.setQuery(result.query);
                switch (graph.initialEval()) {
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
            }
            result.numberOfConfigurations = 0;
            result.numberOfMarkings = 0;
            result.processedEdges = 0;
            result.processedNegationEdges = 0;
            result.exploredConfigurations = 0;
            result.numberOfEdges = 0;
            result.duration = 0;
            result.maxTokens = 0;
            std::pair<bdd, bdd> res;
            if (!solved) {
                if (options.strategy == Strategy::BFS || options.strategy == Strategy::RDFS)
                    res = FCTLSingleSolve(result.query, net, algorithmtype, options.strategy,
                                                    options.stubbornreduction, false, result);
                else
                    res = recursiveSolve(result.query, net, algorithmtype, strategytype, partial_order,
                                                   result, false, options);
            }
            result.result = (res.first == bddtrue);
            if (!result.result) {
                // TODO print counterexample in suitable format. buddy has nice printing facilities,
                // TODO just need to decide on one and sync features back to strings.
            }
            std::cout << "Assignment: " << res.first << '\t' << res.second << std::endl;
            result.print(querynames[qnum], printstatistics, qnum, options, std::cout);
        }
        return ReturnValue::SuccessCode;
    }
}
