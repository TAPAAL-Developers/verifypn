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
    options._seed_offset = (time(nullptr) xor options._seed_offset);
    ColoredPetriNetBuilder cpnBuilder;
    if(parse_model(cpnBuilder, options) != ContinueCode)
    {
        std::cerr << "Error parsing the model" << std::endl;
        return ErrorCode;
    }
    if(options._cpn_overapprox && !cpnBuilder.is_colored())
    {
        std::cerr << "CPN OverApproximation is only usable on colored models" << std::endl;
        return UnknownCode;
    }
    if (options._print_statistics) {
        std::cout << "Finished parsing model" << std::endl;
    }

    //----------------------- Parse Query -----------------------//
    std::vector<std::string> querynames;
    std::vector<Condition_ptr> queries;
    {
        auto ctlStarQueries = read_queries(options, querynames);
        queries = options._logic == options_t::temporal_logic_e::CTL
                ? get_ctl_queries(ctlStarQueries)
                : get_ltl_queries(ctlStarQueries);
    }

    if(options._print_statistics && options._query_reduction_timeout > 0)
    {
        negstat_t stats;
        std::cout << "RWSTATS LEGEND:";
        negstat_t::print_rules(std::cout);
        std::cout << std::endl;
    }

    if(cpnBuilder.is_colored())
    {
        negstat_t stats;
        EvaluationContext context(nullptr, nullptr);
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            queries[qid] = queries[qid]->push_negation(stats, context, false, false, false);
            if(options._print_statistics)
            {
                std::cout << "\nQuery before expansion and reduction: ";
                queries[qid]->to_string(std::cout);
                std::cout << std::endl;

                std::cout << "RWSTATS COLORED PRE:";
                stats.print(std::cout);
                std::cout << std::endl;
            }
        }
    }

    if (options._cpn_overapprox) {
        for (ssize_t qid = queries.size() - 1; qid >= 0; --qid) {
            negstat_t stats;
            EvaluationContext context(nullptr, nullptr);
            auto q = queries[qid]->push_negation(stats, context, false, false, false);
            if (!q->is_reachability() || q->is_loop_sensitive() || stats._negated_fireability) {
                std::cerr << "Warning: CPN OverApproximation is only available for Reachability queries without deadlock, negated fireability and UpperBounds, skipping " << querynames[qid] << std::endl;
                queries.erase(queries.begin() + qid);
                querynames.erase(querynames.begin() + qid);
            }
        }
    }


    if(options._compute_partition){
        cpnBuilder.compute_partition(options._partition_timeout);
    }

    if(options._symmetric_variables){
        cpnBuilder.compute_symmetric_variables();
    }

    if(options._compute_CFP){
        cpnBuilder.compute_place_color_fixpoint(options._max_intervals, options._max_intervals_reduced, options._interval_timeout);
    }


    auto builder = options._cpn_overapprox ? cpnBuilder.strip_colors() : cpnBuilder.unfold();
    print_unfolding_stats(cpnBuilder, options);
    builder.sort();
    std::vector<ResultPrinter::Result> results(queries.size(), ResultPrinter::Result::Unknown);
    ResultPrinter printer(builder, options, querynames);

    //----------------------- Query Simplification -----------------------//

    {
        PetriNetBuilder b2(builder);
        std::unique_ptr<PetriNet> qnet(b2.make_petri_net(false));
        std::unique_ptr<MarkVal[]> qm0(qnet->make_initial_marking());

        if(queries.size() == 0 || context_analysis(cpnBuilder, b2, *qnet.get(), queries) != ContinueCode)
        {
            std::cerr << "Could not analyze the queries" << std::endl;
            return ErrorCode;
        }

        simplify_queries(*qnet, queries, options);

        if(options._query_out_file.size() > 0) {
            output_queries(b2, queries, querynames, options._query_out_file, options._binary_query_io);
        }

        qnet = nullptr;
        qm0 = nullptr;
        print_simplification_results(b2, options, querynames, queries, results);

        if(all_done(results) && options._model_out_file.empty())
            return SuccessCode;

    }

    //--------------------- Apply Net Reduction ---------------//

    if (options._enable_reduction > 0) {
        // Compute how many times each place appears in the query
        builder.start_timer();
        builder.reduce(queries, results, options._enable_reduction,
            options._trace != options_t::trace_level_e::None, nullptr, options._reduction_timeout, options._reductions);
        printer.set_reducer(builder.get_reducer());
    }

    print_reduction_stats(builder, options);

    auto net = std::unique_ptr<PetriNet>(builder.make_petri_net());

    if(options._model_out_file.size() > 0)
    {
        std::fstream file;
        file.open(options._model_out_file, std::ios::out);
        net->to_xml(file);
    }

    if(all_done(results))
        return SuccessCode;

    if (options._replay_trace) {
        return replay_trace(cpnBuilder, builder, *net, queries, results, options);
    }

    if(options._strategy != options_t::search_strategy_e::OverApprox) {

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

        if (!ctl_ids.empty()) {
            options._used_ctl = true;

            auto reachabilityStrategy = options._strategy;

            // Assign indexes
            if(queries.empty() || context_analysis(cpnBuilder, builder, *net, queries) != ContinueCode)
            {
                std::cerr << "An error occurred while assigning indexes" << std::endl;
                return ErrorCode;
            }
            if(options._strategy == options_t::search_strategy_e::DEFAULT)
                options._strategy = options_t::search_strategy_e::DFS;

            for(auto i : ctl_ids)
            {
                auto r = CTL::verify_ctl(*net,
                    queries[i],
                    options);
                CTL::print_ctl_result(querynames[i], r, i, options);
            }

            if (std::find(results.begin(), results.end(), ResultPrinter::Unknown) == results.end()) {
                return v;
            }
            // go back to previous strategy if the program continues
            options._strategy = reachabilityStrategy;
        }
        options._used_ctl = false;

        //----------------------- Verify LTL queries -----------------------//

        if (!ltl_ids.empty() && options._ltl_algorithm != LTL::Algorithm::None) {
            options._used_ltl = true;
            if ((v = context_analysis(cpnBuilder, builder, *net, queries)) != ContinueCode) {
                std::cerr << "Error performing context analysis" << std::endl;
                return v;
            }

            for (auto qid : ltl_ids) {
                auto res = LTL::verify_ltl(*net, queries[qid], querynames[qid], options);
                std::cout << "\nQuery index " << qid << " was solved\n";
                std::cout << "Query is " << (res ? "" : "NOT ") << "satisfied." << std::endl;

            }
            if (all_done(results))
                return SuccessCode;
        }

        //----------------------- Siphon Trap ------------------------//
        run_siphon_trap(*net, queries, results, printer, options);
        if(all_done(results))
            return error_e::SuccessCode;


        //----------------------- Reachability -----------------------//

        //Analyse context again to reindex query
        context_analysis(cpnBuilder, builder, *net, queries);

        if(options._tar && net->number_of_places() > 0)
        {
            //Create reachability search strategy
            TARReachabilitySearch strategy(printer, *net, builder.get_reducer(), options._kbound);

            // Change default place-holder to default strategy
            fprintf(stdout, "Search strategy option was ignored as the TAR engine is called.\n");
            options._strategy = options_t::search_strategy_e::DFS;

            //Reachability search
            strategy.reachable(queries, results,
            options._print_statistics,
            options._trace != options_t::trace_level_e::None);
        }
        else
        {
            // Change default place-holder to default strategy
            if(options._strategy == options_t::search_strategy_e::DEFAULT)
                options._strategy = options_t::search_strategy_e::HEUR;

            ReachabilitySearch strategy(*net, printer, options._kbound);

            //Reachability search
            strategy.reachable(queries, results,
                            options._strategy,
                            options._stubborn_reduction,
                            options._statespace_exploration,
                            options._print_statistics,
                            options._trace != options_t::trace_level_e::None,
                            options.seed());
        }
    }


    return SuccessCode;
}
