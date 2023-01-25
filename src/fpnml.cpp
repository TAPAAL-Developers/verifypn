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

#include <spot/twa/formula2bdd.hh>

#include <filesystem>
#include <random>

namespace fs = std::filesystem;
//using namespace Featured;

struct Options {
    std::string model_dir = "mcc22";
    std::string out_dir = "fmcc22";
    std::string parse_model = "";
    int inv_frequency = 10;
    int feat_count_exp;
    int feat_count_fixed;

    // TODO parameter relating to shape and size of generated formulae.
};

bool is_trivial(const bdd bdd) {
    return (bdd == bddtrue) || (bdd == bddfalse);
}

std::random_device r;

class RNGState {
public:
    RNGState() = default;

    explicit RNGState(const Options& options)
            : rng_(r()), exp_(options.feat_count_exp), nfeats_(1, options.inv_frequency) {}

    size_t nfeatures() { return std::max(2, int(round(exp_(rng_)))); }

    bool should_add_feature() {
        return nfeats_(rng_) == 1;
    }

    auto formula_depth() { return size_t(exp_(rng_)) + 1; }

    int random_feature(int min, int count) {
        std::uniform_int_distribution<> dist{min, min + count};
        return dist(rng_);
    }

    void set_petri_net(const PetriNet* net) {
        net_ = net;
    }

    auto& operator*() { return rng_; }

private:
    const PetriNet* net_;
    std::default_random_engine rng_;
    std::exponential_distribution<> exp_;

    std::uniform_int_distribution<> nfeats_;
};


class FeatureGenerator {
public:
    explicit FeatureGenerator(const Options& options, const spot::bdd_dict_ptr& dict)
        : rng_(options), fnames_(rng_.nfeatures()), dict_(dict) { }

    void annotate_features(PetriNet* net);

    enum Ops : uint32_t {
        Var = 0, And, Or, Not, Imply, Equal, MaxOp__
    };

    virtual ~FeatureGenerator();

    static std::string to_string(Ops op) {
        switch (op) {
            case And: return "And";
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
            case MaxOp__:
                throw base_error{"to_string: invalid Op"};
        }
    }
    spot::bdd_dict_ptr dict_;

private:
    RNGState rng_;
    std::vector<spot::formula> fnames_;

    bdd gen_binary_formula(int maxdepth, int maxtries, Ops op);
    bdd gen_negation_formula(int maxdepth, int maxtries);
    bdd generate_feature(int maxdepth, int maxtries = 5);

    bdd rand_feature_() {
        int num = rng_.random_feature(0, fnames_.size()-1);
        int varnum = dict_->has_registered_proposition(fnames_[num], nullptr);
        if (varnum < 0) {
            throw base_error{"Proposition ", fnames_[num], " not associated with this."};
        }
        std::cerr << "Generating feature variable " << fnames_[num] << ".\n";
        return bdd_ithvar(varnum);
    }
};


void FeatureGenerator::annotate_features(PetriEngine::PetriNet* net) {

    rng_.set_petri_net(net);
    for (auto i = 0; i < fnames_.size(); ++i) {
        auto var_name = "f" + std::to_string(i);
        auto f = spot::formula::ap(var_name);
        fnames_[i] = f;
        dict_->register_proposition(f, nullptr);
        assert(dict_->has_registered_proposition(f, nullptr) != -1);
        assert(dict_->has_registered_proposition(fnames_[i], nullptr) != -1);

        std::cout << "Registering prop " << f << std::endl;
    }

    // maybe dumb; since ops are binary, we want a bit more than nfeats ops, but max of 2 with 2 features.
    int maxdepth = fnames_.size() * fnames_.size() / 2;

    for (int i = 0; i < net->numberOfTransitions(); ++i) {
        if (rng_.should_add_feature()) {
            bdd feat = generate_feature(maxdepth);
            std::cerr << "Generated feature guard: " << spot::bdd_to_formula(feat, dict_) << ".\n";
            if (is_trivial(feat)) {
                // avoid accidentally assigning false to feature.
                net->feat(i) = bddtrue;
            }
            else {
                net->feat(i) = feat;
            }
        }
    }
}

static Options options;

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
        args.push_back(argv[i]);
    }

    for (int i = 0; i < args.size(); ++i) {
        if (args[i] == "--help" || args[i] == "-h") {
            //print_help(); // TODO add help string
        }
        else if (args[i] == "--probability" || args[i] == "-p") {
            double p = std::stod(args[i+1]);
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
            ++i;
        }
        else if (args[i] == "--nfeatures" || args[i] == "-n") {
            // TODO assign nfeatures
        } else if (args[i] == "--lambda" || args[i] == "-l") {
            // TODO assign lambda
        }
        else if (args[i] == "--parse") {
            ++i;
            assert(i != args.size());
            options.parse_model = args[i];
        }
        else {
            options.model_dir = args[i];
        }
    }

    return options;
}

void transform_model(const Options& options) {
    auto f = fs::directory_entry(options.model_dir);
    const fs::path& model_dir{f};
    const std::string& model_name = "model.pnml";
    const std::string& out_name = "featured.pnml";
    if (!fs::exists(model_dir)) {
        throw base_error("Error: Missing directory " + model_dir.string());
    }
    auto ifile = model_dir / model_name;
    auto ofile = model_dir / out_name;
    if (!fs::exists(ifile)) {
        throw base_error{"File " + model_name + " not found in " + model_dir.string()};
    }

    shared_string_set string_set;
    spot::bdd_dict_ptr dict = spot::make_bdd_dict();
    ColoredPetriNetBuilder cpnBuilder{string_set, dict};
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
    auto pnbuilder = cpnBuilder.pt_builder();
    pnbuilder.sort();
    auto pn = pnbuilder.makePetriNet(false);

    FeatureGenerator gen{options, dict};

    std::cout << "Generating features\n";
    gen.annotate_features(pn);
    std::cout << "Done generating features\n";

    std::cout << "Writing featured PN to file " << ofile << std::endl;
    std::ofstream of{ofile};
    pn->toXML(of, gen.dict_);
    std::cout << "Done writing PN to file." << std::endl;
}


bdd FeatureGenerator::generate_feature(int maxdepth, int maxtries) {
    if (maxdepth == 0) {
        std::cerr << "Max depth reached, generating random feature.\n";
        return rand_feature_();
    }

    std::uniform_int_distribution<uint32_t> randop{0, MaxOp__ - 1};

    Ops op = static_cast<Ops>(randop(*rng_));
    std::cerr << "Generating operation " << FeatureGenerator::to_string(op) << ".\n";

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
        case MaxOp__:
            throw base_error{"Error: Invalid value drawn\n"};
    }
}

bdd FeatureGenerator::gen_binary_formula(int maxdepth, int maxtries, Ops op) {
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

bdd FeatureGenerator::gen_negation_formula(int maxdepth, int maxtries) {
    // no need to be smart here.
    return bdd_not(generate_feature(maxdepth - 1, maxtries));
}

FeatureGenerator::~FeatureGenerator() {
}

int main(int argc, char* argv[]) {
    options = parse_options(argc, argv);
    //rng.seed(time(nullptr));

    shared_string_set string_set;
    if (!options.parse_model.empty()) {
        shared_string_set string_set;
        spot::bdd_dict_ptr dict = spot::make_bdd_dict();
        ColoredPetriNetBuilder cpnBuilder{string_set, dict};
        try {
            cpnBuilder.parse_model(options.parse_model.c_str());
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
        auto pn = pnbuilder.makePetriNet(false);
        pn->toXML(std::cout);

        dict->unregister_all_my_variables(nullptr);
        return 0;
    }
    transform_model(options);

    return 0;
}
