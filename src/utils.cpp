

#include "utils.h"
#include "LTL/LTLValidator.h"
#include "LTL/Simplification/SpotToPQL.h"
#include "PetriEngine/PQL/CTLVisitor.h"
#include "PetriEngine/PQL/Expressions.h"
#include "PetriEngine/PQL/PQL.h"
#include "PetriEngine/PQL/PQLParser.h"
#include "PetriEngine/Reachability/ReachabilityResult.h"
#include "PetriEngine/STSolver.h"
#include "PetriEngine/TAR/TARReachability.h"
#include "PetriEngine/TraceReplay.h"
#include "PetriParse/QueryBinaryParser.h"
#include "PetriParse/QueryParser.h"
#include "PetriParse/QueryXMLParser.h"
#include "options.h"

#include <mutex>
#include <thread>

using namespace PetriEngine;
using namespace PetriEngine::PQL;
using namespace PetriEngine::Reachability;

auto all_done(std::vector<PetriEngine::Reachability::ResultPrinter::Result> &results) -> bool {
    if (std::any_of(results.begin(), results.end(), [](auto r) {
            return r != ResultPrinter::SATISFIED && r != ResultPrinter::NOT_SATISFIED;
        })) {
        return false;
    }
    return true;
}

auto context_analysis(const ColoredPetriNetBuilder &cpnBuilder, const PetriNetBuilder &builder,
                      const PetriNet &net, std::vector<std::shared_ptr<Condition>> &queries)
    -> error_e {
    // Context analysis
    ColoredAnalysisContext context(builder.get_place_names(), builder.get_transition_names(), net,
                                   cpnBuilder.get_unfolded_place_names(),
                                   cpnBuilder.get_unfolded_transition_names(),
                                   cpnBuilder.is_colored());
    for (auto &q : queries) {
        q->analyze(context);

        // Print errors if any
        if (context.errors().size() > 0) {
            for (const auto &i : context.errors()) {
                fprintf(stderr, "Query Context Analysis Error: %s\n", i.to_string().c_str());
            }
            return ERROR_CODE;
        }
    }
    return CONTINUE_CODE;
}

auto read_queries(options_t &options, std::vector<std::string> &qstrings)
    -> std::vector<Condition_ptr> {

    std::vector<Condition_ptr> conditions;
    if (!options._statespace_exploration) {
        // Open query file
        std::ifstream qfile(options._queryfile, std::ifstream::in);
        if (!qfile) {
            fprintf(stderr, "Error: Query file \"%s\" couldn't be opened\n", options._queryfile);
            fprintf(stdout, "CANNOT_COMPUTE\n");
            conditions.clear();
            return conditions;
        }

        if (options._query_numbers.size() == 0) {
            std::string querystring; // excluding EF and AG

            // Read everything
            std::stringstream buffer;
            buffer << qfile.rdbuf();
            std::string querystr = buffer.str(); // including EF and AG
            // Parse XML the queries and querystr let be the index of xmlquery

            qstrings.push_back(querystring);
            // Validate query type
            if (querystr.substr(0, 2) != "EF" && querystr.substr(0, 2) != "AG") {
                fprintf(stderr,
                        "Error: Query type \"%s\" not supported, only (EF and AG is supported)\n",
                        querystr.substr(0, 2).c_str());
                return conditions;
            }
            // Check if is invariant
            bool isInvariant = querystr.substr(0, 2) == "AG";

            // Wrap in not if isInvariant
            querystring = querystr.substr(2);
            std::vector<std::string> tmp;
            conditions.emplace_back(parse_query(querystring, tmp));
            if (isInvariant)
                conditions.back() = std::make_shared<AGCondition>(conditions.back());
            else
                conditions.back() = std::make_shared<EFCondition>(conditions.back());
        } else {
            std::vector<query_item_t> queries;
            if (options._binary_query_io & 1) {
                QueryBinaryParser parser;
                if (!parser.parse(qfile, options._query_numbers)) {
                    fprintf(stderr, "Error: Failed parsing binary query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }
                queries = std::move(parser._queries);
            } else {
                QueryXMLParser parser;
                if (!parser.parse(qfile, options._query_numbers)) {
                    fprintf(stderr, "Error: Failed parsing XML query file\n");
                    fprintf(stdout, "DO_NOT_COMPETE\n");
                    conditions.clear();
                    return conditions;
                }
                queries = std::move(parser._queries);
            }

            size_t i = 0;
            for (auto &q : queries) {
                if (!options._query_numbers.empty() && options._query_numbers.count(i) == 0) {
                    ++i;
                    continue;
                }
                ++i;

                if (q._parsing_result == query_item_t::UNSUPPORTED_QUERY) {
                    fprintf(stdout, "The selected query in the XML query file is not supported\n");
                    fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q._id.c_str());
                    continue;
                }
                // fprintf(stdout, "Index of the selected query: %d\n\n", xmlquery);

                conditions.push_back(q._query);
                if (conditions.back() == nullptr) {
                    fprintf(stderr, "Error: Failed to parse query \"%s\"\n",
                            q._id.c_str()); // querystr.substr(2).c_str());
                    fprintf(stdout, "FORMULA %s CANNOT_COMPUTE\n", q._id.c_str());
                    conditions.pop_back();
                }

                qstrings.push_back(q._id);
            }
        }
        qfile.close();
        return conditions;
    } else { // state-space exploration
        std::string querystring = "false";
        std::vector<std::string> empty;
        conditions.push_back(std::make_shared<EFCondition>(parse_query(querystring, empty)));
        return conditions;
    }
}

auto parse_model(AbstractPetriNetBuilder &builder, options_t &options) -> error_e {
    // Load the model
    std::ifstream mfile(options._modelfile, std::ifstream::in);
    if (!mfile) {
        fprintf(stderr, "Error: Model file \"%s\" couldn't be opened\n", options._modelfile);
        fprintf(stdout, "CANNOT_COMPUTE\n");
        return ERROR_CODE;
    }

    // Parse and build the petri net
    PNMLParser parser;
    parser.parse(mfile, &builder);
    options._is_CPN = builder.is_colored();

    // Close the file
    mfile.close();
    return CONTINUE_CODE;
}

void print_reduction_stats(PetriNetBuilder &builder, options_t &options) {
    if (options._print_statistics) {
        if (options._enable_reduction != 0) {

            std::cout << "Size of net before structural reductions: " << builder.number_of_places()
                      << " places, " << builder.number_of_transitions() << " transitions"
                      << std::endl;
            std::cout << "Size of net after structural reductions: "
                      << builder.number_of_places() - builder.removed_places() << " places, "
                      << builder.number_of_transitions() - builder.removed_transitions()
                      << " transitions" << std::endl;
            std::cout << "Structural reduction finished after " << builder.reduction_time()
                      << " seconds" << std::endl;

            std::cout << "\nNet reduction is enabled.\n";
            builder.print_stats(std::cout);
        }
    }
}

void print_unfolding_stats(ColoredPetriNetBuilder &builder, options_t &options) {
    if (!builder.is_colored() && !builder.is_unfolded())
        return;
    if (options._compute_CFP) {
        std::cout << "\nColor fixpoint computed in " << builder.get_fixpoint_time() << " seconds"
                  << std::endl;
        std::cout << "Max intervals used: " << builder.get_max_intervals() << std::endl;
    }

    std::cout << "Size of colored net: " << builder.get_place_count() << " places, "
              << builder.get_transition_count() << " transitions, and " << builder.get_arc_count()
              << " arcs" << std::endl;
    std::cout << "Size of unfolded net: " << builder.get_unfolded_place_count() << " places, "
              << builder.get_unfolded_transition_count() << " transitions, and "
              << builder.get_unfolded_arc_count() << " arcs" << std::endl;
    std::cout << "Unfolded in " << builder.get_unfold_time() << " seconds" << std::endl;
    if (options._compute_partition) {
        std::cout << "Partitioned in " << builder.get_partition_time() << " seconds" << std::endl;
    }
}

auto get_xml_queries(std::vector<std::shared_ptr<Condition>> queries,
                     std::vector<std::string> querynames,
                     std::vector<Reachability::ResultPrinter::Result> results) -> std::string {
    bool cont = false;
    for (auto &result : results) {
        if (result == Reachability::ResultPrinter::CTL) {
            cont = true;
            break;
        }
    }

    if (!cont) {
        return "";
    }

    std::stringstream ss;
    ss << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";

    for (uint32_t i = 0; i < queries.size(); i++) {
        if (!(results[i] == Reachability::ResultPrinter::CTL)) {
            continue;
        }
        ss << "  <property>\n    <id>" << querynames[i]
           << "</id>\n    <description>Simplified</description>\n    <formula>\n";
        queries[i]->to_xml(ss, 3);
        ss << "    </formula>\n  </property>\n";
    }

    ss << "</property-set>\n";

    return ss.str();
}

void write_queries(const std::vector<std::shared_ptr<Condition>> &queries,
                   std::vector<std::string> &querynames, std::vector<uint32_t> &order,
                   std::string &filename, bool binary,
                   const std::unordered_map<std::string, uint32_t> &place_names) {
    std::fstream out;

    if (binary) {
        out.open(filename, std::ios::binary | std::ios::out);
        uint32_t cnt = 0;
        for (const auto &querie : queries) {
            if (querie->is_trivially_true() || querie->is_trivially_false())
                continue;
            ++cnt;
        }
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        cnt = place_names.size();
        out.write(reinterpret_cast<const char *>(&cnt), sizeof(uint32_t));
        for (auto &kv : place_names) {
            out.write(reinterpret_cast<const char *>(&kv.second), sizeof(uint32_t));
            out.write(kv.first.data(), kv.first.size());
            out.write("\0", sizeof(char));
        }
    } else {
        out.open(filename, std::ios::out);
        out << "<?xml version=\"1.0\"?>\n<property-set xmlns=\"http://mcc.lip6.fr/\">\n";
    }

    for (uint32_t j = 0; j < queries.size(); j++) {
        auto i = order[j];
        if (queries[i]->is_trivially_true() || queries[i]->is_trivially_false())
            continue;
        if (binary) {
            out.write(querynames[i].data(), querynames[i].size());
            out.write("\0", sizeof(char));
            queries[i]->to_binary(out);
        } else {
            out << "  <property>\n    <id>" << querynames[i]
                << "</id>\n    <description>Simplified</description>\n    <formula>\n";
            queries[i]->to_xml(out, 3);
            out << "    </formula>\n  </property>\n";
        }
    }

    if (binary == 0) {
        out << "</property-set>\n";
    }
    out.close();
}

auto get_ctl_queries(const std::vector<Condition_ptr> &ctlStarQueries)
    -> std::vector<Condition_ptr> {
    std::vector<Condition_ptr> ctlQueries;
    for (const auto &ctlStarQuery : ctlStarQueries) {
        IsCTLVisitor isCtlVisitor;
        ctlStarQuery->visit(isCtlVisitor);
        if (isCtlVisitor._is_CTL) {
            AsCTL asCtl;
            ctlStarQuery->visit(asCtl);
            ctlQueries.push_back(asCtl._ctl_query);
        } else {
            throw base_error_t("Error: A query could not be translated from CTL* to CTL.");
        }
    }
    return ctlQueries;
}

auto get_ltl_queries(const std::vector<Condition_ptr> &ctlStarQueries)
    -> std::vector<Condition_ptr> {
    std::vector<Condition_ptr> ltlQueries;
    for (const auto &ctlStarQuery : ctlStarQueries) {
        LTL::LTLValidator isLtl;
        if (isLtl.is_ltl(ctlStarQuery)) {
            ltlQueries.push_back(ctlStarQuery);
        } else {
            throw base_error_t("Error: a query could not be translated from CTL* to LTL.");
        }
    }
    return ltlQueries;
}

#ifdef VERIFYPN_MC_Simplification
std::mutex spot_mutex;
#endif

auto simplify_ltl_query(const Condition_ptr &query, const options_t &options,
                        const EvaluationContext &evalContext,
                        SimplificationContext &simplificationContext, std::ostream &out)
    -> Condition_ptr {
    Condition_ptr cond;
    bool wasACond;
    if (std::dynamic_pointer_cast<SimpleQuantifierCondition>(query) != nullptr) {
        wasACond = std::dynamic_pointer_cast<ACondition>(query) != nullptr;
        cond = (*std::dynamic_pointer_cast<SimpleQuantifierCondition>(query))[0];
    } else {
        wasACond = true;
        cond = query;
    }

    {
#ifdef VERIFYPN_MC_Simplification
        std::scoped_lock scopedLock{spot_mutex};
#endif
        cond = LTL::simplify(cond, options);
    }
    negstat_t stats;
    cond = Condition::initial_marking_rewrite([&]() { return cond; }, stats, evalContext, false,
                                              false, true)
               ->push_negation(stats, evalContext, false, false, true);

    if (options._print_statistics) {
        out << "RWSTATS PRE:";
        stats.print(out);
        out << std::endl;
    }

    try {
        cond = (cond->simplify(simplificationContext))
                   ._formula->push_negation(stats, evalContext, false, false, true);
    } catch (std::bad_alloc &ba) {
        throw base_error_t("Query reduction failed.\n",
                         "Exception information: ", ba.what());
    }

    cond = Condition::initial_marking_rewrite(
        [&]() {
#ifdef VERIFYPN_MC_Simplification
            std::scoped_lock scopedLock{spot_mutex};
#endif
            return LTL::simplify(cond->push_negation(stats, evalContext, false, false, true),
                                 options);
        },
        stats, evalContext, false, false, true);

    if (cond->is_trivially_true() || cond->is_trivially_false()) {
        // nothing
    } else if (wasACond) {
        cond = std::make_shared<ACondition>(cond);
    } else {
        cond = std::make_shared<ECondition>(cond);
    }
    if (options._print_statistics) {
        out << "RWSTATS POST:";
        stats.print(out);
        out << std::endl;
        out << "Query after reduction: ";
        cond->to_string(out);
        out << std::endl;
    }
    return cond;
}

void output_queries(const PetriNetBuilder &builder,
                    const std::vector<PetriEngine::PQL::Condition_ptr> &queries,
                    std::vector<std::string> &querynames, std::string filename,
                    uint32_t binary_query_io) {
    std::vector<uint32_t> reorder(queries.size());
    for (uint32_t i = 0; i < queries.size(); ++i)
        reorder[i] = i;
    std::sort(reorder.begin(), reorder.end(), [&](auto a, auto b) {
        if (queries[a]->is_reachability() != queries[b]->is_reachability())
            return queries[a]->is_reachability() > queries[b]->is_reachability();
        if (queries[a]->is_loop_sensitive() != queries[b]->is_loop_sensitive())
            return queries[a]->is_loop_sensitive() < queries[b]->is_loop_sensitive();
        if (queries[a]->contains_next() != queries[b]->contains_next())
            return queries[a]->contains_next() < queries[b]->contains_next();
        return queries[a]->formula_size() < queries[b]->formula_size();
    });
    write_queries(queries, querynames, reorder, filename, binary_query_io & 2,
                  builder.get_place_names());
}

void simplify_queries(const PetriNet &net, std::vector<Condition_ptr> &queries,
                      const options_t &options) {
    // simplification. We always want to do negation-push and initial marking check.
    {
        // simplification. We always want to do negation-push and initial marking check.
        std::atomic<uint32_t> to_handle(queries.size());
        auto begin = std::chrono::high_resolution_clock::now();
        auto end = std::chrono::high_resolution_clock::now();
        std::vector<bool> hadTo(queries.size(), true);
        do {
            auto qt = (options._query_reduction_timeout -
                       std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) /
                      (1 + (to_handle / options._cores));
            if ((to_handle <= options._cores || options._cores == 1) && to_handle > 0)
                qt = (options._query_reduction_timeout -
                      std::chrono::duration_cast<std::chrono::seconds>(end - begin).count()) /
                     to_handle;
            std::atomic<uint32_t> cnt(0);
#ifdef VERIFYPN_MC_Simplification

            std::vector<std::thread> threads;
#endif
            std::vector<std::stringstream> tstream(queries.size());
            uint32_t old = to_handle;
            for (size_t c = 0; c < std::min<uint32_t>(options._cores, old); ++c) {
#ifdef VERIFYPN_MC_Simplification
                threads.push_back(std::thread([&, c]() {
#else
                auto simplify =
                    [&, c]() {
#endif
                    auto &out = tstream[c];
                    LTL::FormulaToSpotSyntax printer{out};
                    while (true) {
                        auto i = cnt++;
                        if (i >= queries.size())
                            return;
                        if (!hadTo[i])
                            continue;
                        hadTo[i] = false;
                        negstat_t stats;
                        EvaluationContext context(net.initial(), &net);

                        if (options._print_statistics && options._query_reduction_timeout > 0) {
                            out << "\nQuery before reduction: ";
                            queries[i]->to_string(out);
                            out << std::endl;
                        }

#ifndef VERIFYPN_MC_Simplification
                        qt = (options._query_reduction_timeout -
                              std::chrono::duration_cast<std::chrono::seconds>(end - begin)
                                  .count()) /
                             (queries.size() - i);
#endif
                        // this is used later, we already know that this is a plain reachability (or
                        // AG)
                        int preSize = queries[i]->formula_size();

                        bool wasAGCPNApprox =
                            dynamic_cast<NotCondition *>(queries[i].get()) != nullptr;
                        if (options._logic == options_t::temporal_logic_e::LTL) {
                            if (options._query_reduction_timeout == 0)
                                continue;
                            SimplificationContext simplificationContext(net.initial(), net, qt,
                                                                        options._lpsolve_timeout);
                            queries[i] = simplify_ltl_query(queries[i], options, context,
                                                            simplificationContext, out);
                            continue;
                        }
                        queries[i] =
                            Condition::initial_marking_rewrite([&]() { return queries[i]; }, stats,
                                                               context, false, false, true)
                                ->push_negation(stats, context, false, false, true);
                        wasAGCPNApprox |= dynamic_cast<NotCondition *>(queries[i].get()) != nullptr;

                        if (options._query_reduction_timeout > 0 && options._print_statistics) {
                            out << "RWSTATS PRE:";
                            stats.print(out);
                            out << std::endl;
                        }

                        if (options._query_reduction_timeout > 0 && qt > 0) {
                            SimplificationContext simplificationContext(net.initial(), net, qt,
                                                                        options._lpsolve_timeout);
                            try {
                                negstat_t stats;
                                queries[i] = (queries[i]->simplify(simplificationContext))
                                                 ._formula->push_negation(stats, context, false,
                                                                          false, true);
                                wasAGCPNApprox |=
                                    dynamic_cast<NotCondition *>(queries[i].get()) != nullptr;
                                if (options._print_statistics) {
                                    out << "RWSTATS POST:";
                                    stats.print(out);
                                    out << std::endl;
                                }
                            } catch (std::bad_alloc &ba) {
                                throw base_error_t("Query reduction failed.\n",
                                                 "Exception information: ", ba.what());
                            }

                            if (options._print_statistics) {
                                out << "\nQuery after reduction: ";
                                queries[i]->to_string(out);
                                out << std::endl;
                            }
                            if (simplificationContext.timeout()) {
                                if (options._print_statistics)
                                    out << "Query reduction reached timeout.\n";
                                hadTo[i] = true;
                            } else {
                                if (options._print_statistics)
                                    out << "Query reduction finished after "
                                        << simplificationContext.get_reduction_time()
                                        << " seconds.\n";
                                --to_handle;
                            }
                        } else if (options._print_statistics) {
                            out << "Skipping linear-programming (-q 0)" << std::endl;
                        }
                        if (options._cpn_overapprox && wasAGCPNApprox) {
                            if (queries[i]->is_trivially_true())
                                queries[i] = std::make_shared<BooleanCondition>(false);
                            else if (queries[i]->is_trivially_false())
                                queries[i] = std::make_shared<BooleanCondition>(true);
                            queries[i]->set_invariant(wasAGCPNApprox);
                        }

                        if (options._print_statistics) {
                            int postSize = queries[i]->formula_size();
                            double redPerc =
                                preSize - postSize == 0
                                    ? 0
                                    : ((double)(preSize - postSize) / (double)preSize) * 100;
                            out << "Query size reduced from " << preSize << " to " << postSize
                                << " nodes ( " << redPerc << " percent reduction).\n";
                        }
                    }
                }
#ifdef VERIFYPN_MC_Simplification
                                              ));
#else
                ;
                simplify();
#endif
            }
#ifndef VERIFYPN_MC_Simplification
            std::cout << tstream[0].str() << std::endl;
            break;
#else
            for (size_t i = 0; i < std::min<uint32_t>(options._cores, old); ++i) {
                threads[i].join();
                std::cout << tstream[i].str();
                std::cout << std::endl;
            }
#endif
            end = std::chrono::high_resolution_clock::now();

        } while (std::any_of(hadTo.begin(), hadTo.end(), [](auto a) { return a; }) &&
                 std::chrono::duration_cast<std::chrono::seconds>(end - begin).count() <
                     options._query_reduction_timeout &&
                 to_handle > 0);
    }
}

auto replay_trace(const ColoredPetriNetBuilder &cpnBuilder, const PetriNetBuilder &builder,
                  const PetriNet &net, std::vector<Condition_ptr> &queries,
                  const std::vector<ResultPrinter::Result> &results, const options_t &options)
    -> error_e {
    if (context_analysis(cpnBuilder, builder, net, queries) != CONTINUE_CODE)
        throw base_error_t("Fatal error assigning indexes");
    std::ifstream replay_file(options._replay_file, std::ifstream::in);
    PetriEngine::TraceReplay replay{replay_file, net, options};
    for (size_t i = 0; i < queries.size(); ++i) {
        if (results[i] == ResultPrinter::UNKNOWN || results[i] == ResultPrinter::CTL ||
            results[i] == ResultPrinter::LTL)
            replay.replay(net, queries[i]);
    }
    return SUCCESS_CODE;
}

void run_siphon_trap(const PetriNet &net, std::vector<Condition_ptr> &queries,
                     std::vector<ResultPrinter::Result> &results, const ResultPrinter &printer,
                     const options_t &options) {
    if (options._siphontrap_timeout > 0) {
        for (uint32_t i = 0; i < results.size(); i++) {
            bool isDeadlockQuery =
                std::dynamic_pointer_cast<DeadlockCondition>(queries[i]) != nullptr;

            if (results[i] == ResultPrinter::UNKNOWN && isDeadlockQuery) {
                STSolver stSolver(printer, net, *queries[i], options._siphon_depth);
                stSolver.solve(options._siphontrap_timeout);
                results[i] = stSolver.print_result();
                if (results[i] == Reachability::ResultPrinter::NOT_SATISFIED &&
                    options._print_statistics) {
                    std::cout << "Query solved by Siphon-Trap Analysis." << std::endl << std::endl;
                }
            }
        }
    }
}

void print_simplification_results(const PetriEngine::PetriNetBuilder &b2, const options_t &options,
                                  const std::vector<std::string> &querynames,
                                  std::vector<Condition_ptr> &queries,
                                  std::vector<ResultPrinter::Result> &results) {
    ResultPrinter p2(b2, options, querynames);
    if (!options._statespace_exploration) {
        for (size_t i = 0; i < queries.size(); ++i) {
            if (queries[i]->is_trivially_true()) {
                results[i] = p2.handle(i, *queries[i], ResultPrinter::SATISFIED).first;
                if (results[i] == ResultPrinter::IGNORE && options._print_statistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl
                              << std::endl;
                } else if (options._print_statistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (queries[i]->is_trivially_false()) {
                results[i] = p2.handle(i, *queries[i], ResultPrinter::NOT_SATISFIED).first;
                if (results[i] == ResultPrinter::IGNORE && options._print_statistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl
                              << std::endl;
                } else if (options._print_statistics) {
                    std::cout << "Query solved by Query Simplification." << std::endl << std::endl;
                }
            } else if (options._strategy == options_t::search_strategy_e::OVER_APPROX) {
                results[i] = p2.handle(i, *queries[i], ResultPrinter::UNKNOWN).first;
                if (options._print_statistics) {
                    std::cout << "Unable to decide if query is satisfied." << std::endl
                              << std::endl;
                }
            } else if (options._noreach || !queries[i]->is_reachability()) {
                results[i] = options._logic == options_t::temporal_logic_e::CTL
                                 ? ResultPrinter::CTL
                                 : ResultPrinter::LTL;
            } else {
                queries[i] = queries[i]->prepare_for_reachability();
            }
        }
    }
}
