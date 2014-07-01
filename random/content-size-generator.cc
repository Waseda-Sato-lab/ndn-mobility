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
 * Author: Jairo Eduardo Lopez
 * Created on: May 9, 2014
 *
 * content-size-generator.cc
 *
 *  Simple command line program to generate a random number using a geometric
 *  distribution. Accepts a size in MB and returns a size in bytes.
 *
 */
#include <ctime>
#include <iostream>
#include <iterator>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/geometric_distribution.hpp>
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
	            		("avg", po::value<double>(), "Set average size (MB) for the geometric distribution")
	            		;

		po::store(po::parse_command_line(ac, av, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			cout << desc << "\n";
			return 0;
		}

		if (! vm.count("avg")) {
			cout << "Size (MB) was not set!.\n";
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

	double average = 1.0/ (vm["avg"].as<double>() * 1048576);
	geometric_distribution<> size_dist(average);

	variate_generator<mt19937_64&, geometric_distribution<> > content_size(gen, size_dist);

	cout << content_size() << endl;

	return 0;
}

