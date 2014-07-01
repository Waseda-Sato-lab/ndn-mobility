/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Copyright (c) 2014 Waseda University
 * Author: Jairo Eduardo Lopez <jairo@ruri.waseda.jp>
 * Created on: June 29, 2014
 * position-generator.cc
 *
 *  Simple command line program to generate a random number using the uniform
 *  distribution. Accepts a minimum size and maximum size. Useful for obtaining
 *  a random position.
 */
#include <ctime>
#include <iostream>
#include <iterator>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/program_options.hpp>

using namespace boost::random;
using namespace std;
namespace po = boost::program_options;

mt19937_64 gen;

int main(int ac, char* av[])
{
	gen.seed(std::time(0) + (long long)getpid() << 32);
	po::variables_map vm;

	try {

		po::options_description desc("Allowed options");
		desc.add_options()
	            		("help", "Produce this help message")
	            		("min", po::value<double>(), "Min value for uniform distribution")
	            		("max", po::value<double>(), "Max value for uniform distribution")
	            		;

		po::store(po::parse_command_line(ac, av, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << "\n";
			return 0;
		}

		if (! vm.count("min")) {
			cout << "Minimum was not set!.\n";
			return 1;
		}
		
		if (! vm.count("max")) {
			cout << "Maximum was not set!.\n";
			return 1;
		}
	}
	catch(std::exception& e) {
		cerr << "error: " << e.what() << "\n";
		return 1;
	}
	catch(...) {
		cerr << "Exception of unknown type!\n";
	}
	
	double minP = vm["min"].as<double>();
	double maxP = vm["max"].as<double>();
	
	uniform_int_distribution<> pos(minP, maxP);

	variate_generator<mt19937_64&, uniform_int_distribution<> > pos_gen(gen, pos);

	cout << pos_gen() << endl;

	return 0;
}
