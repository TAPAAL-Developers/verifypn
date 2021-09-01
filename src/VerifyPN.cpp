/* TAPAAL untimed verification engine verifypn
 * Copyright (C) 2011-2021  Jonas Finnemann Jensen <jopsen@gmail.com>,
 *                          Thomas Søndersø Nielsen <primogens@gmail.com>,
 *                          Lars Kærlund Østergaard <larsko@gmail.com>,
 *                          Jiri Srba <srba.jiri@gmail.com>,
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *
 * CTL Extension
 *                          Peter Fogh <peter.f1992@gmail.com>
 *                          Isabella Kaufmann <bellakaufmann93@gmail.com>
 *                          Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>
 *                          Lasse Steen Jensen <lassjen88@gmail.com>
 *                          Søren Moss Nielsen <soren_moss@mac.com>
 *                          Samuel Pastva <daemontus@gmail.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * Stubborn sets, query simplification, siphon-trap property
 *                          Frederik Meyer Boenneland <sadpantz@gmail.com>
 *                          Jakob Dyhr <jakobdyhr@gmail.com>
 *                          Peter Gjøl Jensen <root@petergjoel.dk>
 *                          Mads Johannsen <mads_johannsen@yahoo.com>
 *                          Jiri Srba <srba.jiri@gmail.com>
 *
 * LTL Extension
 *                          Nikolaj Jensen Ulrik <nikolaj@njulrik.dk>
 *                          Simon Mejlby Virenfeldt <simon@simwir.dk>
 *
 * Color Extension
 *                          Alexander Bilgram <alexander@bilgram.dk>
 *                          Peter Haar Taankvist <ptaankvist@gmail.com>
 *                          Thomas Pedersen <thomas.pedersen@stofanet.dk>
 *                          Andreas H. Klostergaard
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
#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <memory>
#include <utility>
#include <functional>
// #include <filesystem>
// #include <bits/stdc++.h>
// #include <iostream>
// #include <sys/stat.h>
// #include <sys/types.h>
#ifdef VERIFYPN_MC_Simplification
#include <thread>
#include <iso646.h>
#include <mutex>
#endif

#include "utils.h"
#include "options.h"

#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/PQL/Contexts.h"
#include "PetriEngine/Reachability/ReachabilitySearch.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/Reducer.h"
#include "PetriParse/QueryXMLParser.h"
#include "PetriParse/QueryBinaryParser.h"
#include "PetriParse/PNMLParser.h"
#include "PetriEngine/PetriNetBuilder.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/CTLVisitor.h"
#include "options.h"
#include "errorcodes.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/Simplification/Member.h"
#include "PetriEngine/Simplification/LinearPrograms.h"
#include "PetriEngine/Simplification/Retval.h"

#include "CTL/CTLEngine.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/Colored/ColoredPetriNetBuilder.h"
#include "LTL/LTL.h"
#include "PetriEngine/TraceReplay.h"
#include "LTL/LTLMain.h"

#include <atomic>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

int main(int argc, char* argv[]) {

    options_t options;

    error_e v = options.parse(argc, argv);
    if(v != ContinueCode) return v;
    options.print();
    options.seed_offset = (time(NULL) xor options.seed_offset);
    ColoredPetriNetBuilder cpnBuilder;
    if(parseModel(cpnBuilder, options) != ContinueCode)
    {
        std::cerr << "Error parsing the model" << std::endl;
        return ErrorCode;
    }
    if(options.cpnOverApprox && !cpnBuilder.is_colored())
    {
        std::cerr << "CPN OverApproximation is only usable on colored models" << std::endl;
        return UnknownCode;
    }
    if (options.printstatistics) {
        std::cout << "Finished parsing model" << std::endl;
    }

    //----------------------- Parse Query -----------------------//
    std::vector<std::string> querynames;
    std::vector<Condition_ptr> queries;
    {
        auto ctlStarQueries = readQueries(options, querynames);
        queries = options.logic == options_t::TemporalLogic::CTL
                ? getCTLQueries(ctlStarQueries)
                : getLTLQueries(ctlStarQueries);
    }

    if(options.printstatistics && options.queryReductionTimeout > 0)
    {
        negstat_t stats;
        std::cout << "RWSTATS LEGEND:";
        stats.printRules(std::cout);
        std::cout << std::endl;
    }

    if(cpnBuilder.is_colored())
    {
        negstat_t stats;
        EvaluationContext context(nullptr, nullptr);
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            queries[qid] = queries[qid]->pushNegation(stats, context, false, false, false);
            if(options.printstatistics)
            {
                std::cout << "\nQuery before expansion and reduction: ";
                queries[qid]->toString(std::cout);
                std::cout << std::endl;

                std::cout << "RWSTATS COLORED PRE:";
                stats.print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    if (options.cpnOverApprox) {
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            negstat_t stats;
            EvaluationContext context(nullptr, nullptr);
            auto q = queries[qid]->pushNegation(stats, context, false, false, false);
            if (!q->isReachability() || q->isLoopSensitive() || stats.negated_fireability) {
                std::cerr << "Warning: CPN OverApproximation is only available for Reachability queries without deadlock, negated fireability and UpperBounds, skipping " << querynames[qid] << std::endl;
                queries.erase(queries.begin() + qid);
                querynames.erase(querynames.begin() + qid);
            }
        }
    }


    if(options.computePartition){
        cpnBuilder.compute_partition(options.partitionTimeout);
    }
    if(options.symmetricVariables){
        cpnBuilder.compute_symmetric_variables();
        //cpnBuilder.printSymmetricVariables();
    }
    if(options.computeCFP){
        cpnBuilder.compute_place_color_fixpoint(options.max_intervals, options.max_intervals_reduced, options.intervalTimeout);
    }


    auto builder = options.cpnOverApprox ? cpnBuilder.strip_colors() : cpnBuilder.unfold();
    printUnfoldingStats(cpnBuilder, options);
    builder.sort();
    std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
    ResultPrinter printer(&builder, &options, querynames);

    if(options.unfolded_out_file.size() > 0) {
        outputNet(builder, options.unfolded_out_file);
    }

    //----------------------- Query Simplification -----------------------//
    bool alldone = options.queryReductionTimeout > 0;
    PetriNetBuilder b2(builder);
    std::unique_ptr<PetriNet> qnet(b2.make_petri_net(false));
    std::unique_ptr<MarkVal[]> qm0(qnet->makeInitialMarking());
    ResultPrinter p2(&b2, &options, querynames);

    if(options.unfold_query_out_file.size() > 0) {
        outputCompactQueries(builder, b2, qnet.get(), cpnBuilder, queries, querynames, options.unfold_query_out_file);
    }


    if(queries.size() == 0 || contextAnalysis(cpnBuilder, b2, qnet.get(), queries) != ContinueCode)
    {
        std::cerr << "Could not analyze the queries" << std::endl;
        return ErrorCode;
    }

    if(options.query_out_file.size() > 0) {
        outputQueries(builder, queries, querynames, options.query_out_file, options.binary_query_io);

    }

    qnet = nullptr;
    qm0 = nullptr;

    if (!options.statespaceexploration){
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(queries[i]->isTriviallyTrue()){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Satisfied).first;
                if(results[i] == ResultPrinter::Ignore && options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (queries[i]->isTriviallyFalse()) {
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::NotSatisfied).first;
                if(results[i] == ResultPrinter::Ignore &&  options.printstatistics)
                {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
                else if (options.printstatistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (options.strategy == options_t::SearchStrategy::OverApprox){
                results[i] = p2.handle(i, queries[i].get(), ResultPrinter::Unknown).first;
                if (options.printstatistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl << std::endl;
                }
            } else if (options.noreach || !queries[i]->isReachability()) {
                results[i] = options.logic == options_t::TemporalLogic::CTL ? ResultPrinter::CTL : ResultPrinter::LTL;
                alldone = false;
            } else {
                queries[i] = queries[i]->prepareForReachability();
                alldone = false;
            }
        }

        if(alldone && options.model_out_file.size() == 0) return SuccessCode;
    }

    options.queryReductionTimeout = 0;

    //--------------------- Apply Net Reduction ---------------//

    if (options.enablereduction > 0) {
        // Compute how many times each place appears in the query
        builder.start_timer();
        builder.reduce(queries, results, options.enablereduction,
            options.trace != options_t::TraceLevel::None, nullptr, options.reductionTimeout, options.reductions);
        printer.set_reducer(builder.get_reducer());
    }

    printStats(builder, options);

    auto net = std::unique_ptr<PetriNet>(builder.make_petri_net());

    if(options.model_out_file.size() > 0)
    {
        std::fstream file;
        file.open(options.model_out_file, std::ios::out);
        net->toXML(file);
    }

    if(alldone) return SuccessCode;

    if (options.replay_trace) {
        if (contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode) {
            std::cerr << "Fatal error assigning indexes" << std::endl;
            exit(1);
        }
        std::ifstream replay_file(options.replay_file, std::ifstream::in);
        PetriEngine::TraceReplay replay{replay_file, net.get(), options};
        for (size_t i=0; i < queries.size(); ++i) {
            if (results[i] == ResultPrinter::Unknown || results[i] == ResultPrinter::CTL || results[i] == ResultPrinter::LTL)
                replay.replay(net.get(), queries[i]);
        }
        return SuccessCode;
    }

    if(options.doVerification){

        //----------------------- Verify CTL queries -----------------------//
        std::vector<size_t> ctl_ids;
        std::vector<size_t> ltl_ids;
        for(size_t i = 0; i < queries.size(); ++i)
        {
            if(results[i] == ResultPrinter::CTL)
            {
                ctl_ids.push_back(i);
            }
            else if (results[i] == ResultPrinter::LTL) {
                ltl_ids.push_back(i);
            }
        }

        if (options.replay_trace) {
            if (contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode) {
                std::cerr << "Fatal error assigning indexes" << std::endl;
                exit(1);
            }
            std::ifstream replay_file(options.replay_file, std::ifstream::in);
            PetriEngine::TraceReplay replay{replay_file, net.get(), options};
            for (int i : ltl_ids) {
                replay.replay(net.get(), queries[i]);
            }
            return SuccessCode;
        }

        if (!ctl_ids.empty()) {
            options.usedctl=true;
            auto reachabilityStrategy = options.strategy;

            // Assign indexes
            if(queries.empty() || contextAnalysis(cpnBuilder, builder, net.get(), queries) != ContinueCode)
            {
                std::cerr << "An error occurred while assigning indexes" << std::endl;
                return ErrorCode;
            }
            if(options.strategy == options_t::SearchStrategy::DEFAULT)
                options.strategy = options_t::SearchStrategy::DFS;
            v = CTLMain(net.get(),
                options.ctlalgorithm,
                options.strategy,
                options.printstatistics,
                true,
                options.stubbornreduction,
                querynames,
                queries,
                ctl_ids,
                options);

            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return v;
            }
            // go back to previous strategy if the program continues
            options.strategy = reachabilityStrategy;
        }
        options.usedctl = false;

        //----------------------- Verify LTL queries -----------------------//

        if (!ltl_ids.empty() && options.ltlalgorithm != LTL::Algorithm::None) {
            options.usedltl = true;
            if ((v = contextAnalysis(cpnBuilder, builder, net.get(), queries)) != ContinueCode) {
                std::cerr << "Error performing context analysis" << std::endl;
                return v;
            }

            for (auto qid : ltl_ids) {
                auto res = LTL::LTLMain(net.get(), queries[qid], querynames[qid], options);
                std::cout << "\nQuery index " << qid << " was solved\n";
                std::cout << "Query is " << (res ? "" : "NOT ") << "satisfied." << std::endl;

            }
            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return SuccessCode;
            }
        }

        //----------------------- Siphon Trap ------------------------//

        if(options.siphontrapTimeout > 0){
            for (uint32_t i = 0; i < results.size(); i ++) {
                bool isDeadlockQuery = std::dynamic_pointer_cast<DeadlockCondition>(queries[i]) != nullptr;

                if (results[i] == ResultPrinter::Unknown && isDeadlockQuery) {
                    STSolver stSolver(printer, *net, queries[i].get(), options.siphonDepth);
                    stSolver.solve(options.siphontrapTimeout);
                    results[i] = stSolver.printResult();
                    if (results[i] == Reachability::ResultPrinter::NotSatisfied && options.printstatistics) {
                        std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                    }
                }
            }

            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return SuccessCode;
            }
        }
        options.siphontrapTimeout = 0;

        //----------------------- Reachability -----------------------//

        //Analyse context again to reindex query
        contextAnalysis(cpnBuilder, builder, net.get(), queries);

        // Change default place-holder to default strategy
        if(options.strategy == options_t::SearchStrategy::DEFAULT)
            options.strategy = options_t::SearchStrategy::HEUR;

        if(options.tar && net->number_of_places() > 0)
        {
            //Create reachability search strategy
            TARReachabilitySearch strategy(printer, *net, builder.get_reducer(), options.kbound);

            // Change default place-holder to default strategy
            fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
            options.strategy = options_t::SearchStrategy::DFS;

            //Reachability search
            strategy.reachable(queries, results,
                    options.printstatistics,
                    options.trace != options_t::TraceLevel::None);
        }
        else
        {
            ReachabilitySearch strategy(*net, printer, options.kbound);

            //Reachability search
            strategy.reachable(queries, results,
                            options.strategy,
                            options.stubbornreduction,
                            options.statespaceexploration,
                            options.printstatistics,
                            options.trace != options_t::TraceLevel::None,
                            options.seed());
        }
    }


    return SuccessCode;
}
