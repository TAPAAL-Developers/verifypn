
#include "options.h"

#include <algorithm>
#include <array>
#include <cstring>

auto explode(std::string const &s) -> std::vector<std::string> {
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, ',');) {
        result.push_back(std::move(token));
        if (result.back().empty())
            result.pop_back();
    }

    return result;
}

void options_t::print(std::ostream &optionsOut) const {
    if (!_print_statistics) {
        return;
    }

    if (_strategy == search_strategy_e::BFS) {
        optionsOut << "\nSearch=BFS";
    } else if (_strategy == search_strategy_e::DFS) {
        optionsOut << "\nSearch=DFS";
    } else if (_strategy == search_strategy_e::HEUR) {
        optionsOut << "\nSearch=HEUR";
    } else if (_strategy == search_strategy_e::RDFS) {
        optionsOut << "\nSearch=RDFS";
    } else {
        optionsOut << "\nSearch=OverApprox";
    }

    if (_trace != trace_level_e::None) {
        optionsOut << ",Trace=ENABLED";
    } else {
        optionsOut << ",Trace=DISABLED";
    }

    if (_kbound > 0) {
        optionsOut << ",Token_Bound=" << _kbound;
    }

    if (_statespace_exploration) {
        optionsOut << ",State_Space_Exploration=ENABLED";
    } else {
        optionsOut << ",State_Space_Exploration=DISABLED";
    }

    if (_enable_reduction == 0) {
        optionsOut << ",Structural_Reduction=DISABLED";
    } else if (_enable_reduction == 1) {
        optionsOut << ",Structural_Reduction=AGGRESSIVE";
    } else {
        optionsOut << ",Structural_Reduction=KBOUND_PRESERVING";
    }

    optionsOut << ",Struct_Red_Timout=" << _reduction_timeout;

    if (_stubborn_reduction) {
        optionsOut << ",Stubborn_Reduction=ENABLED";
    } else {
        optionsOut << ",Stubborn_Reduction=DISABLED";
    }

    if (_query_reduction_timeout > 0) {
        optionsOut << ",Query_Simplication=ENABLED,QSTimeout=" << _query_reduction_timeout;
    } else {
        optionsOut << ",Query_Simplication=DISABLED";
    }

    if (_siphontrap_timeout > 0) {
        optionsOut << ",Siphon_Trap=ENABLED,SPTimeout=" << _siphontrap_timeout;
    } else {
        optionsOut << ",Siphon_Trap=DISABLED";
    }

    optionsOut << ",LPSolve_Timeout=" << _lpsolve_timeout;

    if (_used_ctl) {
        if (_ctlalgorithm == CTL::CZero) {
            optionsOut << ",CTLAlgorithm=CZERO";
        } else {
            optionsOut << ",CTLAlgorithm=LOCAL";
        }
    } else if (_used_ltl) {
        switch (_ltl_algorithm) {
        case LTL::Algorithm::NDFS:
            optionsOut << ",LTLAlgorithm=NDFS";
            break;
        case LTL::Algorithm::Tarjan:
            optionsOut << ",LTLAlgorithm=Tarjan";
            break;
        case LTL::Algorithm::None:
            optionsOut << ",LTLAlgorithm=None";
            break;
        }
    }

    optionsOut << std::endl;
}

auto options_t::parse(int argc, char *argv[]) -> error_e {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--k-bound") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_kbound) != 1 || _kbound < 0) {
                fprintf(stderr, "Argument Error: Invalid number of tokens \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--search-strategy") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing search strategy after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            char *s = argv[++i];
            if (strcmp(s, "BestFS") == 0)
                _strategy = search_strategy_e::HEUR;
            else if (strcmp(s, "BFS") == 0)
                _strategy = search_strategy_e::BFS;
            else if (strcmp(s, "DFS") == 0)
                _strategy = search_strategy_e::DFS;
            else if (strcmp(s, "RDFS") == 0)
                _strategy = search_strategy_e::RDFS;
            else if (strcmp(s, "OverApprox") == 0)
                _strategy = search_strategy_e::OverApprox;
            else {
                fprintf(stderr, "Argument Error: Unrecognized search strategy \"%s\"\n", s);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--query-reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_query_reduction_timeout) != 1 ||
                _query_reduction_timeout < 0) {
                fprintf(stderr, "Argument Error: Invalid query reduction timeout argument \"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "--interval-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_interval_timeout) != 1 || _interval_timeout < 0) {
                fprintf(stderr, "Argument Error: Invalid fixpoint timeout argument \"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "--partition-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_partition_timeout) != 1 || _partition_timeout < 0) {
                fprintf(stderr, "Argument Error: Invalid fixpoint timeout argument \"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--lpsolve-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_lpsolve_timeout) != 1 || _lpsolve_timeout < 0) {
                fprintf(stderr, "Argument Error: Invalid LPSolve timeout argument \"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-e") == 0 ||
                   strcmp(argv[i], "--state-space-exploration") == 0) {
            _statespace_exploration = true;
            _compute_partition = false;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-statistics") == 0) {
            _print_statistics = false;
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--trace") == 0) {
            if (argc > i + 1) {
                if (strcmp("1", argv[i + 1]) == 0) {
                    _trace = trace_level_e::Transitions;
                } else if (strcmp("2", argv[i + 1]) == 0) {
                    _trace = trace_level_e::Full;
                } else {
                    _trace = trace_level_e::Full;
                    continue;
                }
                ++i;
            } else {
                _trace = trace_level_e::Full;
            }
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--xml-queries") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            std::vector<std::string> q = explode(argv[++i]);
            for (auto &qn : q) {
                int32_t n;
                if (sscanf(qn.c_str(), "%d", &n) != 1 || n <= 0) {
                    std::cerr << "Error in query numbers : " << qn << std::endl;
                } else {
                    _query_numbers.insert(--n);
                }
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--reduction") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_enable_reduction) != 1 || _enable_reduction < 0 ||
                _enable_reduction > 3) {
                fprintf(stderr, "Argument Error: Invalid reduction argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if (_enable_reduction == 3) {
                _reductions.clear();
                std::vector<std::string> q = explode(argv[++i]);
                for (auto &qn : q) {
                    int32_t n;
                    if (sscanf(qn.c_str(), "%d", &n) != 1 || n < 0 || n > 10) {
                        std::cerr << "Error in reduction rule choice : " << qn << std::endl;
                        return ErrorCode;
                    } else {
                        _reductions.push_back(n);
                    }
                }
            }
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--reduction-timeout") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_reduction_timeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid reduction timeout argument \"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "--seed-offset") == 0) {
            if (sscanf(argv[++i], "%u", &_seed_offset) != 1) {
                fprintf(stderr, "Argument Error: Invalid seed offset argument \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-p") == 0 ||
                   strcmp(argv[i], "--partial-order-reduction") == 0) {
            _stubborn_reduction = false;
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--siphon-trap") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &_siphontrap_timeout) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-trap timeout \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "--siphon-depth") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &_siphon_depth) != 1) {
                fprintf(stderr, "Argument Error: Invalid siphon-depth count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "-tar") == 0) {
            _tar = true;

        } else if (strcmp(argv[i], "--max-intervals") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%d", &_max_intervals) != 1 || _max_intervals < 0) {
                fprintf(stderr,
                        "Argument Error: Invalid number of max intervals in first argument\"%s\"\n",
                        argv[i]);
                return ErrorCode;
            }
            if (i != argc - 1) {
                if (sscanf(argv[++i], "%d", &_max_intervals_reduced) != 1 ||
                    _max_intervals_reduced < 0) {
                    fprintf(stderr,
                            "Argument Error: Invalid number of max intervals in second argument "
                            "\"%s\"\n",
                            argv[i]);
                    return ErrorCode;
                }
            }
        } else if (strcmp(argv[i], "--write-simplified") == 0) {
            _query_out_file = std::string(argv[++i]);
        } else if (strcmp(argv[i], "--binary-query-io") == 0) {
            if (sscanf(argv[++i], "%u", &_binary_query_io) != 1 || _binary_query_io > 3) {
                fprintf(stderr, "Argument Error: Invalid binary-query-io value \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        } else if (strcmp(argv[i], "--write-reduced") == 0) {
            _model_out_file = std::string(argv[++i]);
        } else if (strcmp(argv[i], "--write-buchi") == 0) {
            _buchi_out_file = std::string(argv[++i]);
            if (argc > i + 1) {
                if (strcmp(argv[i + 1], "dot") == 0) {
                    _buchi_out_type = LTL::BuchiOutType::Dot;
                } else if (strcmp(argv[i + 1], "hoa") == 0) {
                    _buchi_out_type = LTL::BuchiOutType::HOA;
                } else if (strcmp(argv[i + 1], "spin") == 0) {
                    _buchi_out_type = LTL::BuchiOutType::Spin;
                } else
                    continue;
                ++i;
            }
        } else if (strcmp(argv[i], "--compress-aps") == 0) {
            if (argc <= i + 1 || strcmp(argv[i + 1], "1") == 0) {
                _ltl_compress_aps = atomic_compression_e::Full;
                ++i;
            } else if (strcmp(argv[i + 1], "0") == 0) {
                _ltl_compress_aps = atomic_compression_e::None;
                ++i;
            }
        } else if (strcmp(argv[i], "--spot-optimization") == 0) {
            if (argc == i + 1) {
                std::cerr << "Missing argument to --spot-optimization\n";
                return ErrorCode;
            } else if (strcmp(argv[i + 1], "1") == 0) {
                _buchi_optimization = buchi_optimization_e::Low;
            } else if (strcmp(argv[i + 1], "2") == 0) {
                _buchi_optimization = buchi_optimization_e::Medium;
            } else if (strcmp(argv[i + 1], "3") == 0) {
                _buchi_optimization = buchi_optimization_e::High;
            } else {
                std::cerr << "Invalid argument " << argv[i] << " to --spot-optimization\n";
                return ErrorCode;
            }
            ++i;
        } else if (strcmp(argv[i], "--trace-replay") == 0) {
            _replay_trace = true;
            _replay_file = std::string(argv[++i]);
        }

#ifdef VERIFYPN_MC_Simplification
        else if (strcmp(argv[i], "-z") == 0) {
            if (i == argc - 1) {
                fprintf(stderr, "Missing number after \"%s\"\n\n", argv[i]);
                return ErrorCode;
            }
            if (sscanf(argv[++i], "%u", &_cores) != 1) {
                fprintf(stderr, "Argument Error: Invalid cores count \"%s\"\n", argv[i]);
                return ErrorCode;
            }
        }
#endif
        else if (strcmp(argv[i], "-noreach") == 0) {
            _noreach = true;
        } else if (strcmp(argv[i], "-ctl") == 0) {
            _logic = temporal_logic_e::CTL;
            if (argc > i + 1) {
                if (strcmp(argv[i + 1], "local") == 0) {
                    _ctlalgorithm = CTL::Local;
                } else if (strcmp(argv[i + 1], "czero") == 0) {
                    _ctlalgorithm = CTL::CZero;
                } else {
                    fprintf(stderr, "Argument Error: Invalid ctl-algorithm type \"%s\"\n",
                            argv[i + 1]);
                    return ErrorCode;
                }
                i++;
            }
        } else if (strcmp(argv[i], "-ltl") == 0) {
            _logic = temporal_logic_e::LTL;
            if (argc > i + 1) {
                if (strcmp(argv[i + 1], "ndfs") == 0) {
                    _ltl_algorithm = LTL::Algorithm::NDFS;
                } else if (strcmp(argv[i + 1], "tarjan") == 0) {
                    _ltl_algorithm = LTL::Algorithm::Tarjan;
                } else if (strcmp(argv[i + 1], "none") == 0) {
                    _ltl_algorithm = LTL::Algorithm::None;
                } else {
                    continue;
                }
                i++;
            }
        } else if (strcmp(argv[i], "--ltl-por") == 0) {
            if (argc == i + 1) {
                std::cerr << "Missing argument to --ltl-por\n";
                return ErrorCode;
            } else if (strcmp(argv[i + 1], "classic") == 0) {
                _ltl_por = ltl_partial_order_e::Visible;
            } else if (strcmp(argv[i + 1], "reach") == 0) {
                _ltl_por = ltl_partial_order_e::AutomatonReach;
            } else if (strcmp(argv[i + 1], "mix") == 0) {
                _ltl_por = ltl_partial_order_e::VisibleReach;
            } else if (strcmp(argv[i + 1], "automaton") == 0) {
                _ltl_por = ltl_partial_order_e::FullAutomaton;
            } else if (strcmp(argv[i + 1], "none") == 0) {
                _ltl_por = ltl_partial_order_e::None;
            } else {
                std::cerr << "Unrecognized argument " << argv[i + 1] << " to --ltl-por\n";
                return ErrorCode;
            }
            ++i;
        } else if (strcmp(argv[i], "--ltl-heur") == 0) {
            if (argc == i + 1) {
                std::cerr << "Missing argument to --ltl-heur\n";
                return ErrorCode;
            } else {
                _ltl_heuristic = argv[i + 1];
            }
            ++i;
        } else if (strcmp(argv[i], "-noweak") == 0) {
            _ltl_use_weak = false;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--cpn-overapproximation") == 0) {
            _cpn_overapprox = true;
        } else if (strcmp(argv[i], "--disable-cfp") == 0) {
            _compute_CFP = false;
        } else if (strcmp(argv[i], "--disable-partitioning") == 0) {
            _compute_partition = false;
        } else if (strcmp(argv[i], "--disable-symmetry-vars") == 0) {
            _symmetric_variables = false;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(
                "Usage: verifypn [options] model-file query-file\n"
                "A tool for answering CTL, LTL and reachability queries\n"
                "for weighted P/T Petri nets extended with inhibitor arcs.\n"
                "\n"
                "Options:\n"
                "  -k, --k-bound <number of tokens>     Token bound, 0 to ignore (default)\n"
                "  -t, --trace                          Provide XML-trace to stderr\n"
                "  -s, --search-strategy <strategy>     Search strategy:\n"
                "                                       - BestFS       Heuristic search (default)\n"
                "                                       - BFS          Breadth first search\n"
                "                                       - DFS          Depth first search (CTL "
                "default)\n"
                "                                       - RDFS         Random depth first search\n"
                "                                       - OverApprox   Linear Over Approx\n"
                "  --seed-offset <number>               Extra noise to add to the seed of the "
                "random number generation\n"
                "  -e, --state-space-exploration        State-space exploration only (query-file "
                "is irrelevant)\n"
                "  -x, --xml-query <query index>        Parse XML query file and verify query of a "
                "given index\n"
                "  -r, --reduction <type>               Change structural net reduction:\n"
                "                                       - 0  disabled\n"
                "                                       - 1  aggressive reduction (default)\n"
                "                                       - 2  reduction preserving k-boundedness\n"
                "                                       - 3  user defined reduction sequence, eg "
                "-r 3 0,1,2,3 to use rules A,B,C,D only, and in that order\n"
                "  -d, --reduction-timeout <timeout>    Timeout for structural reductions in "
                "seconds (default 60)\n"
                "  -q, --query-reduction <timeout>      Query reduction timeout in seconds "
                "(default 30)\n"
                "                                       write -q 0 to disable query reduction\n"
                "  --interval-timeout <timeout>         Time in seconds before the max intervals "
                "is halved (default 10)\n"
                "                                       write --interval-timeout 0 to disable "
                "interval limits\n"
                "  --partition-timeout <timeout>        Timeout for color partitioning in seconds "
                "(default 5)\n"
                "  -l, --lpsolve-timeout <timeout>      LPSolve timeout in seconds, default 10\n"
                "  -p, --partial-order-reduction        Disable partial order reduction (stubborn "
                "sets)\n"
                "  --ltl-por <type>                     Select partial order method to use with "
                "LTL engine (default reach).\n"
                "                                       - reach      apply reachability stubborn "
                "sets in Büchi states\n"
                "                                                    that represent reachability "
                "subproblems,\n"
                "                                       - classic    classic stubborn set method.\n"
                "                                                    Only applicable with formulae "
                "that do not \n"
                "                                                    contain the next-step "
                "operator.\n"
                "                                       - mix        mix of reach and classic - "
                "use reach when applicable,\n"
                "                                                    classic otherwise.\n"
                "                                       - automaton  apply fully Büchi-guided "
                "stubborn set method.\n"
                "                                       - none       disable stubborn reductions "
                "(equivalent to -p).\n"
                "  --ltl-heur <spec>                    Select distance metric for LTL heuristic "
                "search\n"
                "                                       Use --ltl-heur-help to see specification "
                "grammar.\n"
                "                                       Defaults to 'aut'.\n"
                "  -a, --siphon-trap <timeout>          Siphon-Trap analysis timeout in seconds "
                "(default 0)\n"
                "      --siphon-depth <place count>     Search depth of siphon (default 0, which "
                "counts all places)\n"
                "  -n, --no-statistics                  Do not display any statistics (default is "
                "to display it)\n"
                "  -h, --help                           Display this help message\n"
                "  -v, --version                        Display version information\n"
                "  -ctl <type>                          Verify CTL properties\n"
                "                                       - local     Liu and Smolka's on-the-fly "
                "algorithm\n"
                "                                       - czero     local with certain zero "
                "extension (default)\n"
                "  -ltl [<type>]                        Verify LTL properties (default tarjan). If "
                "omitted the queries are assumed to be CTL.\n"
                "                                       - ndfs      Nested depth first search "
                "algorithm\n"
                "                                       - tarjan    On-the-fly Tarjan's algorithm\n"
                "                                       - none      Run preprocessing steps only.\n"
                "  -noweak                              Disable optimizations for weak Büchi "
                "automata when doing \n"
                "                                       LTL model checking. Not recommended.\n"
                "  -noreach                             Force use of CTL/LTL engine, even when "
                "queries are reachability.\n"
                "                                       Not recommended since the reachability "
                "engine is faster.\n"
                "  -c, --cpn-overapproximation          Over approximate query on Colored Petri "
                "Nets (CPN only)\n"
                "  --disable-cfp                        Disable the computation of possible colors "
                "in the Petri Net (CPN only)\n"
                "  --disable-partitioning               Disable the partitioning of colors in the "
                "Petri Net (CPN only)\n"
                "  --disable-symmetry-vars              Disable search for symmetric variables "
                "(CPN only)\n"
#ifdef VERIFYPN_MC_Simplification
                "  -z <number of cores>                 Number of cores to use (currently only "
                "query simplification)\n"
#endif
                "  -tar                                 Enables Trace Abstraction Refinement for "
                "reachability properties\n"
                "  --max-intervals <interval count>     The max amount of intervals kept when "
                "computing the color fixpoint\n"
                "                  <interval count>     Default is 250 and then after "
                "<interval-timeout> second(s) to 5\n"
                "  --write-simplified <filename>        Outputs the queries to the given file "
                "after simplification\n"
                "  --write-reduced <filename>           Outputs the model to the given file after "
                "structural reduction\n"
                "  --write-unfolded-net <filename>      Outputs the model to the given file before "
                "structural reduction but after unfolding\n"
                "  --write-unfolded-queries <filename>  Outputs the queries to the given file "
                "before query reduction but after unfolding\n"
                "  --binary-query-io <0,1,2,3>          Determines the input/output format of the "
                "query-file\n"
                "                                       - 0 MCC XML format for Input and Output\n"
                "                                       - 1 Input is binary, output is XML\n"
                "                                       - 2 Output is binary, input is XML\n"
                "                                       - 3 Input and Output is binary\n"
                "  --write-buchi <filename> [<format>]  Valid for LTL. Write the generated buchi "
                "automaton to file. Formats:\n"
                "                                       - dot   (default) Write the buchi in "
                "GraphViz Dot format\n"
                "                                       - hoa   Write the buchi in the Hanoi "
                "Omega-Automata Format\n"
                "                                       - spin  Write the buchi in the spin model "
                "checker format.\n"
                "  --compress-aps                       Enable compression of atomic propositions "
                "in LTL.\n"
                "                                       For some queries this helps reduce the "
                "overhead of query\n"
                "                                       simplification and Büchi construction, but "
                "gives worse\n"
                "                                       results since there is less opportunity "
                "for optimizations.\n"
                "  --noverify                           Disable verification e.g. for getting "
                "unfolded net\n"
                "  --trace-replay <file>                Replays a trace as output by the --trace "
                "option.\n"
                "                                       The trace is verified against the provided "
                "model and query.\n"
                "                                       Mainly useful for debugging.\n"
                "  --spot-optimization <1,2,3>          The optimization level passed to Spot for "
                "Büchi automaton creation.\n"
                "                                       1: Low (default), 2: Medium, 3: High\n"
                "                                       Using optimization levels above 1 may "
                "cause exponential blowups and is not recommended.\n"
                "\n"
                "Return Values:\n"
                "  0   Successful, query satisfiable\n"
                "  1   Unsuccesful, query not satisfiable\n"
                "  2   Unknown, algorithm was unable to answer the question\n"
                "  3   Error, see stderr for error message\n"
                "\n"
                "VerifyPN is an untimed CTL verification engine for TAPAAL.\n"
                "TAPAAL project page: <http://www.tapaal.net>\n");
            return SuccessCode;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("VerifyPN (untimed verification engine for TAPAAL) %s\n", VERIFYPN_VERSION);
            printf("Copyright (C) 2011-2021\n");
            printf("                        Alexander Bilgram <alexander@bilgram.dk>\n");
            printf("                        Frederik Meyer Boenneland <sadpantz@gmail.com>\n");
            printf("                        Jakob Dyhr <jakobdyhr@gmail.com>\n");
            printf("                        Peter Fogh <peter.f1992@gmail.com>\n");
            printf("                        Jonas Finnemann Jensen <jopsen@gmail.com>\n");
            printf("                        Lasse Steen Jensen <lassjen88@gmail.com>\n");
            printf("                        Peter Gjøl Jensen <root@petergjoel.dk>\n");
            printf("                        Tobias Skovgaard Jepsen <tobiasj1991@gmail.com>\n");
            printf("                        Mads Johannsen <mads_johannsen@yahoo.com>\n");
            printf("                        Kenneth Yrke Jørgensen <kenneth@yrke.dk>\n");
            printf("                        Isabella Kaufmann <bellakaufmann93@gmail.com>\n");
            printf("                        Andreas Hairing Klostergaard <kloster92@me.com>\n");
            printf("                        Søren Moss Nielsen <soren_moss@mac.com>\n");
            printf("                        Thomas Søndersø Nielsen <primogens@gmail.com>\n");
            printf("                        Samuel Pastva <daemontus@gmail.com>\n");
            printf("                        Thomas Pedersen <thomas.pedersen@stofanet.dk>\n");
            printf("                        Jiri Srba <srba.jiri@gmail.com>\n");
            printf("                        Peter Haar Taankvist <ptaankvist@gmail.com>\n");
            printf("                        Nikolaj Jensen Ulrik <nikolaj@njulrik.dk>\n");
            printf("                        Simon Mejlby Virenfeldt <simon@simwir.dk>\n");
            printf("                        Lars Kærlund Østergaard <larsko@gmail.com>\n");
            printf("GNU GPLv3 or later <http://gnu.org/licenses/gpl.html>\n");
            return SuccessCode;
        } else if (strcmp(argv[i], "--ltl-heur-help") == 0) {
            printf("Heuristics for LTL model checking are specified using the following grammar:\n"
                   "  heurexp : {'aut' | 'automaton'}\n"
                   "          | {'dist' | 'distance'}\n"
                   "          | {'fc' | 'firecount' | 'fire-count'} <threshold>?\n"
                   "          | '(' heurexp ')'\n"
                   "          | 'sum' <weight>? heurexp <weight?> heurexp\n"
                   "          | 'sum' '(' <weight>? heurexp ',' <weight>? heurexp ')'\n"
                   "Example strings:\n"
                   "  - aut - use the automaton heuristic for verification.\n"
                   "  - sum dist fc 1000 - use the sum of distance heuristic and fire count "
                   "heuristic with threshold 1000.\n"
                   "Weight for sum and threshold for fire count are optional and integral.\n"
                   "Sum weights default to 1 (thus plain sum), and default fire count threshold is "
                   "5000.\n");
            return SuccessCode;
        } else if (_modelfile == nullptr) {
            _modelfile = argv[i];
        } else if (_queryfile == nullptr) {
            _queryfile = argv[i];
        } else {
            fprintf(stderr, "Argument Error: Unrecognized option \"%s\"\n", _modelfile);
            return ErrorCode;
        }
    }
    // Print parameters
    if (_print_statistics) {
        std::cout << std::endl << "Parameters: ";
        for (int i = 1; i < argc; i++) {
            std::cout << argv[i] << " ";
        }
        std::cout << std::endl;
    }

    if (_statespace_exploration) {
        // for state-space exploration some options are mandatory
        _enable_reduction = 0;
        _kbound = 0;
        _query_reduction_timeout = 0;
        _lpsolve_timeout = 0;
        _siphontrap_timeout = 0;
        _stubborn_reduction = false;
        //        outputtrace = false;
    }

    //----------------------- Validate Arguments -----------------------//

    // Check for model file
    if (!_modelfile) {
        fprintf(stderr, "Argument Error: No model-file provided\n");
        return ErrorCode;
    }

    // Check for query file
    if (!_modelfile && !_statespace_exploration) {
        fprintf(stderr, "Argument Error: No query-file provided\n");
        return ErrorCode;
    }

    // Check for compatibility with LTL model checking
    if (_logic == temporal_logic_e::LTL) {
        if (_tar) {
            std::cerr << "Argument Error: -tar is not compatible with LTL model checking."
                      << std::endl;
            return ErrorCode;
        }
        if (_siphontrap_timeout != 0) {
            std::cerr
                << "Argument Error: -a/--siphon-trap is not compatible with LTL model checking."
                << std::endl;
            return ErrorCode;
        }
        if (_siphon_depth != 0) {
            std::cerr << "Argument Error: --siphon-depth is not compatible with LTL model checking."
                      << std::endl;
            return ErrorCode;
        }
        std::array ltlStrategies{search_strategy_e::DFS, search_strategy_e::RDFS,
                                 search_strategy_e::HEUR};

        if (_strategy != search_strategy_e::DEFAULT && _strategy != search_strategy_e::OverApprox) {
            if (std::find(std::begin(ltlStrategies), std::end(ltlStrategies), _strategy) ==
                std::end(ltlStrategies)) {
                std::cerr << "Argument Error: Unsupported search strategy for LTL. Supported "
                             "values are DFS, RDFS, and BestFS."
                          << std::endl;

                return ErrorCode;
            }
        }
    }

    if (false && _replay_trace && _logic != temporal_logic_e::LTL) {
        std::cerr << "Argument Error: Trace replay_trace is only supported for LTL model checking."
                  << std::endl;
        return ErrorCode;
    }

    return ContinueCode;
}