/*
 * simple_writing_performance_test.cpp
 *
 *  Created on: 28 Mar 2017
 *      Author: gnx91527
 *
 *
 *      This test will try to only stress the HDF5 writing speed without
 *      adding any other overheads to the the test.
 *
 *      A single block of memory will be allocated with the correct chunk
 *      size.
 *      The file will be written with the same chunk size, single dataset
 *      with all of the same values.
 *      The time taken to write each chunk recorded, along with the overall
 *      time taken for the test to complete.
 */
#include <sys/time.h>
#include <boost/program_options.hpp>
#include <hdf5_hl.h>
#include <iostream>

#include "hdf5_file_operations.h"

namespace opt = boost::program_options;


int main(int argc, char** argv)
{
	int iterations = 0;
	hsize_t alignment = 0;
	hsize_t chunkSize = 0;
	hsize_t dsetSize = 0;
	int fnum = 1;
	int flush = 0;
	std::stringstream fname;

	// Declare a group of options that will allowed only on the command line
	opt::options_description options("Performance test options");
	options.add_options()
			("help,h", "Print this help message")
			("fnum,n", opt::value<unsigned int>()->default_value(1), "File number")
			("flush,f", opt::value<unsigned int>()->default_value(0), "Flush after each chunk")
			("gpfs,g", opt::value<unsigned int>()->default_value(0), "Use local(0), GPFS0(1) or gpfs0(2)")
			("alignment,a", opt::value<hsize_t>()->default_value(4*1024*1024), "Chunk alignment (bytes)")
			("chunk,c", opt::value<hsize_t>()->default_value(4*1024*1024), "Chunk size (bytes)")
			("iterations,i", opt::value<unsigned int>()->default_value(10), "Number of chunks to write");


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

	if (vm.count("alignment")){
		alignment = vm["alignment"].as<hsize_t>();
	}

	if (vm.count("chunk")){
		chunkSize = vm["chunk"].as<hsize_t>();
	}

	if (vm.count("iterations")){
		iterations = vm["iterations"].as<unsigned int>();
	}

	if (vm.count("flush")){
		flush = vm["flush"].as<unsigned int>();
	}

	// Allocate the buffer large enough to hold 1 chunk worth of data
	int buffSize = chunkSize / sizeof(uint32_t);
	uint32_t buffer[buffSize];
	memset(buffer, buffSize, 1);

	double times[iterations];
    printf("Creating file: %s\n", fname.str().c_str());

	timeval curTime;
	gettimeofday(&curTime, NULL);
	unsigned long micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
	// Create the file with the correct alignment
	hid_t fileID = createFile(fname.str(), alignment);
	gettimeofday(&curTime, NULL);
	micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
	double tTime = (double)micro / 1000000.0;
	printf("File creation time: %f\n", tTime);

	// Create the dataset (chunk size is dimension so supply buffSize)
	hid_t dsetID = createDataset(fileID, "uint32", H5T_NATIVE_UINT32, buffSize, iterations);

	//timeval curTime;
	gettimeofday(&curTime, NULL);
	//unsigned long micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
	micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;

	for (int loop=0; loop < iterations; loop++){
		timeval curTime2;
		gettimeofday(&curTime2, NULL);
		unsigned long micro2 = curTime2.tv_sec*(u_int64_t)1000000+curTime2.tv_usec;

		// Extend the dataset
		extendDataset(dsetID, dsetSize, buffSize);

		writeDataset(dsetID, dsetSize, buffSize, buffer, 1, flush);
		dsetSize += buffSize;

		gettimeofday(&curTime2, NULL);
		micro2 = curTime2.tv_sec*(u_int64_t)1000000+curTime2.tv_usec - micro2;
		double tTime2 = (double)micro2 / 1000000.0;
		times[loop] = tTime2;
		if (tTime2 > 1.0){
			time_t nowtime;
			struct tm *nowtm;
			char tmbuf[64], buf[64];
			nowtime = curTime2.tv_sec;
			nowtm = localtime(&nowtime);
			strftime(tmbuf, sizeof tmbuf, "%Y-%m-%d %H:%M:%S", nowtm);
			printf("Timestamp: %s.%06ld\n", tmbuf, curTime2.tv_usec);
			printf("Outlier chunk write time: %f\n", tTime2);
			printf("  Chunk Size: %d\n", chunkSize);
			printf("  Iteration number: %d\n", loop);
		}

	}
	gettimeofday(&curTime, NULL);
	micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
	//double tTime = (double)micro / 1000000.0;
	tTime = (double)micro / 1000000.0;
	printf("Processing rate %.6f MB/s\n", double(chunkSize*iterations) / (1024*1024) / tTime);


	gettimeofday(&curTime, NULL);
	micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
	closeFile(fileID);
	gettimeofday(&curTime, NULL);
	micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
	tTime = (double)micro / 1000000.0;
	printf("File close time %f s\n", tTime);


	FILE *f = fopen("/home/gnx91527/performance.log", "w");
	for (int loop=0; loop < iterations; loop++){
		fprintf(f, "%d %f\n", loop, times[loop]);
	}
	fclose(f);
	return 0;
}






