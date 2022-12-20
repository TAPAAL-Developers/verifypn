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

#include <filesystem>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

struct Options {
    std::string model_dir = "mcc22";
    std::string out_dir = "fmcc22";
    int inv_frequency = 10;
    int feat_count_exp;
    int feat_count_fixed;

    // TODO parameter relating to shape and size of generated formulae.
};

Options parse_options(int argc, char* argv[]) {
    Options options;
    po::options_description desc("Allowed options");
    po::positional_options_description p;
    p.add("input-dir", 1);
    p.add("output-dir"1, 1);
    desc.add_options()
        ("help", "produce help message")
        ("probability,p", po::value<double>(), "Per transition probability of feature."
                          "If p > 0, interpret as 1/p, otherwith interpret as p.")
        ("nfeatures,n", po::value<int>()->default_value(0), "Number of features. If unset or 0, drawn randomly from exponential distribution")
        ("lambda,l", po::value<int>()->default_value(3), "Parameter for exponential function giving number of features")
        ;

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
            p = 1/p;
            options.inv_frequency = floor(p);
        }
        else {
            options.inv_frequency = floor(p);
        }
    }
    options.model_dir = vm["input-dir"].as<std::string>();
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);

    int opt;
    po::options_description desc("Allowed options");

    shared_string_set string_set;
    ColoredPetriNetBuilder cpnBuilder(string_set);
    try {
        cpnBuilder.parse_model(options.modelfile);
        //options.isCPN = cpnBuilder.isColored(); // TODO: this is really nasty, should be moved in a refactor
    } catch (const base_error &err) {
        throw base_error("CANNOT_COMPUTE\nError parsing the model\n", err.what());
    }

}