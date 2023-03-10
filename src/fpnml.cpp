/* Copyright (C) 2022  Nikolaj J. Ulrik <nikolaj@njulrik.dk>,
 * 
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


#include "PetriEngine/Colored/PnmlWriter.h"
#include "PetriEngine/PQL/PQL.h"
#include "VerifyPN.h"
#include "FCTL/FeaturedPN/PetriNetSerializer.h"

#include <spot/twa/formula2bdd.hh>

#include <filesystem>
#include <random>
#include <variant>

namespace fs = std::filesystem;
//using namespace Featured;

namespace Featured {
    struct FixedFeatures {
        int val;
    };
    struct MaxFeatures {
        int val;
    };

    struct Options {
        std::string model_dir = "mcc2022";
        std::optional<std::string> out_dir = std::nullopt;
        std::string parse_model = "";
        double inv_frequency = 0.25;
        //int feat_count_exp;
        std::variant<FixedFeatures, MaxFeatures> feat_count = MaxFeatures{8};
        int lambda;

        // TODO parameter relating to shape and size of generated formulae.

        bool verbose=true;
        bool enumerate = false;
        std::optional<size_t> seed;
    };

    bool is_trivial(const bdd bdd) {
        return (bdd == bddtrue) || (bdd == bddfalse);
    }

    std::random_device r;

    class RNGState {
    public:
        RNGState() = delete;

        explicit RNGState(const Options& options)
                : options_(options), rng_(options.seed.has_value() ? *options.seed : r()), gen_feat_(options.inv_frequency) {

        }

        size_t nfeatures() {
            auto feat_count = std::visit([&](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, FixedFeatures>) {
                    return arg.val;
                } else if (std::is_same_v<T, MaxFeatures>) {
                    if (arg.val < 2)
                        throw base_error{"Error: Refusing to generate less than 2 features."};
                    std::uniform_int_distribution<> dist{2, arg.val};
                    int n = dist(rng_);
                    return n;
                }
            }, options_.feat_count);
            nfeats_ = std::uniform_int_distribution<>{0, feat_count - 1};

            return feat_count;
        }

        bool should_add_feature() {
            return gen_feat_(rng_);
        }

        bool coin_flip() {
            return coin_(rng_);
        }

        auto formula_depth() { return size_t(exp_(rng_)) + 1; }

        int random_feature(int min, int count) {
            return (*nfeats_)(rng_);
        }

        void set_petri_net(const PetriNet* net) {
            net_ = net;
        }

        [[nodiscard]] const PetriNet* get_petri_net() const {
            return net_;
        }

        auto& operator*() { return rng_; }

    private:
        const Options& options_;
        const PetriNet* net_;
        std::default_random_engine rng_;
        std::exponential_distribution<> exp_;
        std::bernoulli_distribution gen_feat_;
        std::bernoulli_distribution coin_{0.5};

        std::optional<std::uniform_int_distribution<> > nfeats_;
    };


    class FeatureGenerator {
    public:
        explicit FeatureGenerator(const Options& options, const spot::bdd_dict_ptr& dict)
                : dict_(dict), rng_(options), fnames_(rng_.nfeatures()), verbose_(options.verbose) {}

        void annotate_features(PetriNet* net);

        enum Ops : uint32_t {
            Var = 0, And, Or, Not, Imply, Equal, MaxOp_
        };

        virtual ~FeatureGenerator();

        static std::string to_string(Ops op) {
            switch (op) {
                case And:
                    return "And";
                case Or:
                    return "Or";
                case Not:
                    return "Not";
                case Imply:
                    return "Imply";
                case Equal:
                    return "Equal";
                case Var:
                    return "Var";
                case MaxOp_:
                    throw base_error{"to_string: invalid Op"};
            }
        }

        spot::bdd_dict_ptr dict_;

        void print_stats(std::ostream& os = std::cout) const {
            os << "Stats from generation:\n"
               << "  Number of features: " << stats_.num_features << "\n"
               << "  Number of annotated transitions: " << stats_.n_featured_transitions << "/"
               << rng_.get_petri_net()->numberOfTransitions()
               << std::setprecision(3) << " ("
               << 100 * double(stats_.n_featured_transitions) / rng_.get_petri_net()->numberOfTransitions() << "%)"
               << "\n";
        }

    private:
        struct Stats {
            size_t n_featured_transitions = 0;
            size_t num_features = 0;
        };

        Stats stats_;
        RNGState rng_;
        std::vector<spot::formula> fnames_;
        bool verbose_;


        bdd gen_binary_formula(size_t maxdepth, int maxtries, Ops op);

        bdd gen_negation_formula(size_t maxdepth, int maxtries);

        bdd generate_feature(size_t maxdepth, int maxtries = 5);

        bdd rand_feature_() {
            int num = rng_.random_feature(0, fnames_.size() - 1);
            int varnum = dict_->has_registered_proposition(fnames_[num], this);
            if (varnum < 0) {
                throw base_error{"Proposition ", fnames_[num], " not associated with this."};
            }
            if (verbose_) {
                std::cerr << "Generating feature variable " << fnames_[num] << ".\n";
            }
            return rng_.coin_flip() ? bdd_ithvar(varnum) : bdd_nithvar(varnum);
        }

        void set_petri_net(const PetriNet* net) {
            rng_.set_petri_net(net);
        }
    };


    void print_help(const std::vector<std::string>& argv);

    void FeatureGenerator::annotate_features(PetriEngine::PetriNet* net) {

        set_petri_net(net);
        for (size_t i = 0; i < fnames_.size(); ++i) {
            auto var_name = "f" + std::to_string(i);
            auto f = spot::formula::ap(var_name);
            fnames_[i] = f;
            dict_->register_proposition(f, this);
            assert(dict_->has_registered_proposition(f, this) != -1);
            assert(dict_->has_registered_proposition(fnames_[i], this) != -1);

            //std::cout << "Registering prop " << f << std::endl;
        }
        stats_.num_features = fnames_.size();

        // maybe dumb; since ops are binary, we want a bit more than nfeats ops, but max of 2 with 2 features.
        // if single feature this generates to random variable.
        size_t maxdepth = 2 * fnames_.size() - 2;
        std::cout << "Generating with " << stats_.num_features << " features and maximum formula depth of " << maxdepth
                  << ".\n";

        for (uint32_t i = 0; i < net->numberOfTransitions(); ++i) {
            if (rng_.should_add_feature()) {
                bdd feat = generate_feature(maxdepth);
                if (verbose_) {
                    std::cerr << "Generated feature guard: " << spot::bdd_to_formula(feat, dict_) << ".\n";
                }
                if (is_trivial(feat)) {
                    // avoid accidentally assigning false to feature.
                    net->feat(i) = bddtrue;
                } else {
                    net->feat(i) = feat;
                    ++stats_.n_featured_transitions;
                }
            }
        }
    }


    template<typename T>
    T consume_args(const std::vector<std::string>& argv, size_t& i) {
        if (i >= argv.size() - 1) {
            throw base_error{"parse_options: No argument to consume"};
        }
        ++i;
        if constexpr (std::is_integral_v<T>) {
            return std::stoi(argv[i]);
        } else if constexpr (std::is_floating_point_v<T>) {
            return std::stod(argv[i]);
        } else if constexpr (std::is_same_v<T, std::string>) {
            return argv[i];
        } else {
            static_assert(std::is_void_v<T> && false, "Does not know how to parse type.");
        }
    }


    Options parse_options(int argc, char* argv[]) {
        Options options;
        /*po::options_description desc("Allowed options");
        po::positional_options_description p;
        p.add("input-dir", 1);
        p.add("output-dir", 1);
        desc.add_options()
                ("help", "produce help message")
                ("probability,p", po::value<double>(), "Per transition probability of feature."
                                                       "If p > 0, interpret as 1/p, otherwith interpret as p.")
                ("nfeatures,n", po::value<int>()->default_value(0),
                 "Number of features. If unset or 0, drawn randomly from exponential distribution")
                ("lambda,l", po::value<int>()->default_value(3),
                 "Parameter for exponential function giving number of features");

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv)
                          .options(desc)
                          .positional(p)
                          .run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << "\n";
            exit(0);
        }
        if (vm.count("probability")) {
            auto p = vm["probability"].as<double>();
            if (p < 0) {
                std::cerr << "Error: Negative probability" << p << "given.\n";
                exit(1);
            }
            if (p < 1) {
                p = 1 / p;
                options.inv_frequency = floor(p);
            } else {
                options.inv_frequency = floor(p);
            }
        }
        options.model_dir = vm["input-dir"].as<std::string>();*/

        std::vector<std::string> args;
        for (int i = 1; i < argc; ++i) {
            args.emplace_back(argv[i]);
        }

        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--help" || args[i] == "-h") {
                print_help(args);
            } else if (args[i] == "--probability" || args[i] == "-p") {
                auto p = consume_args<double>(args, i);
                if (p < 0) {
                    std::cerr << "Error: Negative probability" << p << "given.\n";
                    exit(1);
                }
                if (p > 1) {
                    p = 1 / p;
                    options.inv_frequency = p;
                } else {
                    options.inv_frequency = p;
                }
            } else if (args[i] == "--lambda" || args[i] == "-l") {
                options.lambda = consume_args<int>(args, i);
            } else if (args[i] == "--parse") {
                options.parse_model = consume_args<std::string>(args, i);
            } else if (args[i] == "--enumerate") {
                options.enumerate = true;
                options.parse_model = consume_args<std::string>(args, i);
            } else if (args[i] == "--verbose") {
                options.verbose = true;
            } else if (args[i] == "--quiet" || args[i] == "-q") {
                options.verbose = false;
            } else if (args[i] == "--max_features") {
                options.feat_count = MaxFeatures{consume_args<int>(args, i)};
            } else if (args[i] == "--num-features" || args[i] == "-n") {
                options.feat_count = FixedFeatures{consume_args<int>(args, i)};
            } else if (args[i] == "--max-depth") {
                throw base_error{"--max-depth not yet implemented, please implement me."};
            } else if (args[i] == "--odir" || args[i] == "--output-dir" || args[i] == "-o") {
                options.out_dir = consume_args<std::string>(args, i);
            } else if (args[i] == "--seed" || args[i] == "-s") {
                options.seed = consume_args<size_t>(args,i);
            }
            else {
                options.model_dir = args[i];
            }
        }

        return options;
    }

    constexpr auto model_name = "model.pnml";
    constexpr auto out_name = "featured.pnml";

    void print_help(const std::vector<std::string>& argv) {
        std::cout << "Usage:\t" << argv[0] << "[parameters] <model_dir> | \n"
                  << "\t" << argv[0] << "--parse <model-file>\n"
                  << "\n"
                  << "Annotate a P/T Petri Net in .pnml with randomly generated features.\n"
                  << "The argument is a directory from e.g. the MCC* datasets, assuming "
                  << model_name //TODO rephrase if this is parameterised.
                  << "contains the original Petri net, and writes the resulting annotated net to " << out_name << ".\n"
                  << "\n"
                  << "Parameters are drawn from the following:\n"
                  << "  -n|--nfeatures NUM             TODO implement\n"
                  << "  -l|--lambda NUM                TODO implement\n"
                  << "  -p|--probability NUM           ";


    }

    void transform_model(const Options& options) {
        auto f = fs::directory_entry(options.model_dir);
        const fs::path& model_dir{f};
        if (!fs::exists(model_dir)) {
            throw base_error("Error: Missing directory " + model_dir.string());
        }
        auto out_dir = std::filesystem::path(options.out_dir.value_or(model_dir));
        auto ifile = model_dir / model_name;
        auto ofile = out_dir / out_name;
        if (!fs::exists(ifile)) {
            throw base_error{"File ", model_name, " not found in ", model_dir.string()};
        }

        shared_string_set string_set;
        spot::bdd_dict_ptr dict = spot::make_bdd_dict();
        ColoredPetriNetBuilder cpnBuilder{string_set, dict};
        std::cout << "Parsing model file " << ifile.string() << "... ";
        try {
            cpnBuilder.parse_model(ifile.string());
        }
        catch (const base_error& err) {
            throw base_error("CANNOT_COMPUTE\nError parsing the model\n", err.what());
        }
        if (cpnBuilder.isColored()) {
            // TODO this might actually be worth doing? idk
            throw base_error("Do not want to feature-ise colored nets");
        }
        std::cout << "Done!\n";
        auto pnbuilder = cpnBuilder.pt_builder();
        pnbuilder.sort();
        auto pn = pnbuilder.makePetriNet(false);

        FeatureGenerator gen{options, dict};

        std::cout << "Starting feature generation...\n";
        gen.annotate_features(pn);
        std::cout << "Done generating features.\n";
        gen.print_stats();

        std::cout << "Writing featured PN to file " << ofile << "... ";
        std::ofstream of{ofile};
        pn->toXML(of, gen.dict_);
        std::cout << "Done!" << std::endl;
    }


    bdd FeatureGenerator::generate_feature(size_t maxdepth, int maxtries) {
        if (maxdepth == 0) {
            if (verbose_) {
                std::cerr << "Max depth reached, generating random feature.\n";
            }
            return rand_feature_();
        }

        std::uniform_int_distribution<uint32_t> randop{0, MaxOp_ - 1};

        Ops op = static_cast<Ops>(randop(*rng_));
        if (verbose_) {
            std::cerr << "Generating operation " << FeatureGenerator::to_string(op) << ".\n";
        }

        switch (op) {
            case Var:
                return rand_feature_();
            case And:
            case Or:
            case Imply:
            case Equal:
                return gen_binary_formula(maxdepth, maxtries, op);
            case Not:
                return gen_negation_formula(maxdepth, maxtries);
            case MaxOp_:
                throw base_error{"Error: Invalid value drawn\n"};
        }
    }

    bdd FeatureGenerator::gen_binary_formula(size_t maxdepth, int maxtries, Ops op) {
        bdd ret;
        int triesleft = maxtries;
        do {
            auto l = generate_feature(maxdepth - 1, maxtries);
            auto r = generate_feature(maxdepth - 1, maxtries);

            switch (op) {
                case And:
                    ret = bdd_and(l, r);
                    break;
                case Or:
                    ret = bdd_or(l, r);
                    break;
                case Imply:
                    ret = bdd_imp(l, r);
                    break;
                case Equal:
                    ret = bdd_biimp(l, r);
                    break;
                default:
                    throw base_error{"gen_binary_formula: Invalid operation ", op};
            }
            --triesleft;
        } while (triesleft > 0 && is_trivial(ret));
        return ret;
    }

    bdd FeatureGenerator::gen_negation_formula(size_t maxdepth, int maxtries) {
        // no need to be smart here.
        return bdd_not(generate_feature(maxdepth - 1, maxtries));
    }

    FeatureGenerator::~FeatureGenerator() {
        dict_->unregister_all_my_variables(this);
    }

}
static Featured::Options options;

using namespace Featured;
int main(int argc, char* argv[]) {
    options = parse_options(argc, argv);
    //rng.seed(time(nullptr));

    shared_string_set string_set;
    if (!options.parse_model.empty()) {
        std::filesystem::path path{options.parse_model};
        if (!std::filesystem::exists(path)) {
            throw base_error{"Input file ", options.parse_model, " does not exist."};
        }
        if (std::filesystem::is_directory(path)) {
            std::cerr << "Given file is a directory. Using filename featured.pnml.\n";
            path = path / out_name;
        }
        shared_string_set string_set;
        spot::bdd_dict_ptr dict = spot::make_bdd_dict();
        ColoredPetriNetBuilder cpnBuilder{string_set, dict};
        try {
            cpnBuilder.parse_model(path.c_str());
        }
        catch (const base_error& err) {
            throw base_error("CANNOT_COMPUTE\nError parsing the model\n", err.what());
        }
        if (cpnBuilder.isColored()) {
            // TODO this might actually be worth doing? idk
            throw base_error("Do not want to feature-ise colored nets");
        }
        auto pnbuilder = cpnBuilder.pt_builder();
        pnbuilder.sort();
        if (options.enumerate) {
            PetriNetSerializer serializer{pnbuilder, options.verbose};
            serializer.serialize_features(options.parse_model);
            return 0;
        } else {
            auto pn = pnbuilder.makePetriNet(false);
            pn->toXML(std::cout);

            dict->unregister_all_my_variables(nullptr);
            return 0;
        }
    }
    transform_model(options);

    return 0;
}