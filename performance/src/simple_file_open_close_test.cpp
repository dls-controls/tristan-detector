/*
 * simple_file_open_close_test.cpp
 *
 *  Created on: 4 Apr 2017
 *      Author: gnx91527
 */
#include <sys/time.h>
#include <boost/program_options.hpp>
#include <iostream>

#include "hdf5_file_operations.h"

namespace opt = boost::program_options;

int main(int argc, char** argv)
{
	int iterations = 0;
	hsize_t dsetSize = 0;
	int dataset = 0;
	int fnum = 1;
	int flush = 0;

	// Declare a group of options that will allowed only on the command line
	opt::options_description options("Performance test options");
	options.add_options()
			("help,h", "Print this help message")
			("fnum,n", opt::value<unsigned int>()->default_value(1), "File number")
			("gpfs,g", opt::value<unsigned int>()->default_value(0), "Use local(0), GPFS0(1) or gpfs0(2)")
			("dataset,d", opt::value<unsigned int>()->default_value(0), "1 - Create Dataset")
			("iterations,i", opt::value<unsigned int>()->default_value(10), "Number of iterations of the test");


	// Parse the command line options
	opt::variables_map vm;
	opt::store(opt::parse_command_line(argc, argv, options), vm);
	opt::notify(vm);

	// If the command-line help option was given, print help and exit
	if (vm.count("help")){
	    std::cout << "usage: simple_writing_performance_test [options]" << std::endl << std::endl;
		std::cout << options << std::endl;
		return 1;
	}

	if (vm.count("fnum")){
		fnum = vm["fnum"].as<unsigned int>();
	}

	if (vm.count("dataset")){
		dataset = vm["dataset"].as<unsigned int>();
	}

	if (vm.count("iterations")){
		iterations = vm["iterations"].as<unsigned int>();
	}


	double times[iterations];

	for (int loop=0; loop < iterations; loop++){
		std::stringstream fname;
		if (vm.count("gpfs")){
			if (vm["gpfs"].as<unsigned int>() == 0){
				fname << "/scratch/gnx91527/latrd/latrd_" << fnum << ".h5";
			}
			if (vm["gpfs"].as<unsigned int>() == 1){
				fname << "/mnt/GPFS01/tmp/gnx91527/latrd/latrd_" << fnum << ".h5";
			}
			if (vm["gpfs"].as<unsigned int>() == 2){
				fname << "/mnt/gpfs02/testdir/gnx91527/latrd/latrd_" << fnum << ".h5";
			}
		} else {
			fname << "/scratch/gnx91527/latrd/latrd_" << fnum << ".h5";
		}

		timeval curTime;
		gettimeofday(&curTime, NULL);
		unsigned long micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
		// Create the file with 4MB alignment
		hid_t fileID = createFile(fname.str(), (4*1024*1024));
		gettimeofday(&curTime, NULL);
		micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
		double tTime = (double)micro / 1000000.0;
		times[loop] = tTime;
		//printf("File creation time: %f\n", tTime);

		if (dataset ==1){
			gettimeofday(&curTime, NULL);
			micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
			// Create the dataset (chunk size is dimension so supply buffSize)
			hid_t dsetID = createDataset(fileID, "uint32", H5T_NATIVE_UINT32, (4*1024*1024), iterations);
			gettimeofday(&curTime, NULL);
			micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
			tTime = (double)micro / 1000000.0;
			//printf("Dataset creation time: %f\n", tTime);
			closeDataset(dsetID);
		}

		gettimeofday(&curTime, NULL);
		micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
		closeFile(fileID);
		gettimeofday(&curTime, NULL);
		micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
		tTime = (double)micro / 1000000.0;
		//printf("File close time: %f\n", tTime);
	}

	FILE *f = fopen("/home/gnx91527/performance.log", "w");
	for (int loop=0; loop < iterations; loop++){
		fprintf(f, "%d %f\n", loop, times[loop]);
	}
	fclose(f);

	return 0;
}


