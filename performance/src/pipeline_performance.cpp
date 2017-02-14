/*
 * pipeline_performance.cpp
 *
 *  Created on: 13 Feb 2017
 *      Author: gnx91527
 */

#include <iostream>
#include <math.h>
#include <sstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <boost/program_options.hpp>
#include <boost/asio/io_service.hpp>
#include <queue>
#include "tbb/pipeline.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"
#include "BufferBuilder.h"
#include "TBBPacket.h"
#include "TBBProcessedPacket.h"

namespace opt = boost::program_options;

class MyInputFilter: public tbb::filter {
public:
    MyInputFilter(BufferBuilder *bb);
    ~MyInputFilter();
private:
    BufferBuilder *bb_;
    TBBPacket* next_packet;
    /*override*/ void* operator()(void*);
};

MyInputFilter::MyInputFilter(BufferBuilder *bb) :
    filter(serial_in_order),
	bb_(bb),
    next_packet(TBBPacket::allocate(bb->getBufferWordCount()))
{
}

MyInputFilter::~MyInputFilter()
{
    next_packet->free();
}

void* MyInputFilter::operator()(void*)
{
	if (!bb_->hasNextBufferPtr()){
		return NULL;
    } else {
        // Have more buffers to process.
    	next_packet->append((u_int64_t *)bb_->getNextBufferPtr(), bb_->getBufferWordCount());
        TBBPacket& pkt = *next_packet;
        next_packet = TBBPacket::allocate(bb_->getBufferWordCount());
        return &pkt;
    }
}

//! Filter that changes each decimal number to its square.
class MyTransformFilter: public tbb::filter {
public:
    MyTransformFilter();
    /*override*/void* operator()( void* item );
};

MyTransformFilter::MyTransformFilter() :
    tbb::filter(parallel)
{}

/*override*/void* MyTransformFilter::operator()( void* item ) {
    TBBPacket& input = *static_cast<TBBPacket*>(item);
    TBBProcessedPacket& out = *TBBProcessedPacket::allocate(1022);

    // Add terminating null so that strtol works right even if number is at end of the input.
	u_int64_t csTime = 0;
	u_int64_t csPrevTime = 0;
	u_int64_t timeStamp = 0;
	u_int64_t tsFine = 0;
	int mf = 0;
	int mc = 0;
	int mpc = 0;
	u_int64_t word = 0;
	u_int64_t localEventCount = 0;
	u_int64_t localCtrlCount = 0;
	u_int64_t localTimeCount = 0;
	int bufferWordCount = input.size();
	u_int64_t *bPtr = input.begin();
	for (int wIndex = 0; wIndex < bufferWordCount; wIndex++){
		word = bPtr[wIndex];
		//printf("Word: %x\n", word);
		if (word & 0x8000000000000000){
			//printf("Control word found\n");
			localCtrlCount++;
			if (((word >> 58) & 0x03F) == 0x20){
				//printf("Course time word found\n");
				localTimeCount++;
				if (csTime != (word & 0x000FFFFFFFFFFFF8)){
					csPrevTime = csTime;
					csTime = (word & 0x000FFFFFFFFFFFF8);
					mpc = mc;
					mc = (csTime >> 22) & 0x03;
				}
				//printf("Course time %lu\n", csTime);
				//printf("Prev time %lu\n", csPrevTime);
			}
		} else {
			// This is an event word
			localEventCount++;
			//evtArray[wIndex] = ((word >> 39) & 0x0000000000FFFFFF);
			//timeStamp = fullTimestamp(mc, mpc, csPrevTime, csTime, ((word >> 14) & 0x0000000000FFFFFF));
///////////////////////////////////
			tsFine = ((word >> 14) & 0x0000000000FFFFFF);
			mf = (tsFine >> 22) & 0x03;
			if ((mc == mf) || (mc+1 == mf)){
				timeStamp = (csTime & 0x0FFFFFFF000000) + tsFine;
			} else {
				if ((mpc == mf) || (mpc+1 == mf)){
					timeStamp = (csPrevTime & 0x0FFFFFFF000000) + tsFine;
				}
			}
			//tsArray[wIndex] = timeStamp;
///////////////////////////////////
			//enArray[wIndex] = word & 0x0000000000003FFF;
			out.append(((word >> 39) & 0x0000000000FFFFFF), timeStamp, (word & 0x0000000000003FFF));
		}
	}
	//printf("processBuffer exit\n");
	//recordEvent(localEventCount, localCtrlCount, localTimeCount);
    input.free();
    return &out;
}

//! Filter that writes each buffer to a file.
class MyOutputFilter: public tbb::filter
{
public:
    MyOutputFilter();
    /*override*/void* operator()( void* item );
};

MyOutputFilter::MyOutputFilter() :
    tbb::filter(serial_in_order)
{
}

void* MyOutputFilter::operator()( void* item )
{
    TBBProcessedPacket& out = *static_cast<TBBProcessedPacket*>(item);
    //out.print(123);
    out.free();
    return NULL;
}

double run_pipeline(BufferBuilder *bb, int nthreads)
{
    // Create the pipeline
    tbb::pipeline pipeline;

    // Create file-reading writing stage and add it to the pipeline
    MyInputFilter input_filter(bb);
    pipeline.add_filter(input_filter);

    // Create squaring stage and add it to the pipeline
    MyTransformFilter transform_filter;
    pipeline.add_filter(transform_filter);

    // Create file-writing stage and add it to the pipeline
    MyOutputFilter output_filter;
    pipeline.add_filter(output_filter);

    // Run the pipeline
    tbb::tick_count t0 = tbb::tick_count::now();
    // Need more than one token in flight per thread to keep all threads
    // busy; 2-4 works
    pipeline.run( nthreads );
    tbb::tick_count t1 = tbb::tick_count::now();


    printf("time = %g\n", (t1-t0).seconds());

    return (t1-t0).seconds();
}


int main(int argc, char** argv)
{
	int packetsPerBuffer = 0;
	int buffers = 0;
	int iterations = 0;
	int tests = 0;
	int threads = 1;

	// Declare a group of options that will allowed only on the command line
	opt::options_description options("Performance test options");
	options.add_options()
			("help,h", "Print this help message")
			("packets,p", opt::value<unsigned int>()->default_value(1), "Number of packets per buffer")
			("buffers,b", opt::value<unsigned int>()->default_value(8192), "Number of buffers to create")
			("iterations,i", opt::value<unsigned int>()->default_value(10), "Number of iterations per test")
			("tests,t", opt::value<unsigned int>()->default_value(20), "Number of times to execute test")
			("threads,s", opt::value<unsigned int>()->default_value(10), "Number of threads to spawn");

	// Parse the command line options
	opt::variables_map vm;
	opt::store(opt::parse_command_line(argc, argv, options), vm);
	opt::notify(vm);

	// If the command-line help option was given, print help and exit
	if (vm.count("help")){
	    std::cout << "usage: frameReceiver [options]" << std::endl << std::endl;
		std::cout << options << std::endl;
		return 1;
	}

	if (vm.count("packets")){
		packetsPerBuffer = vm["packets"].as<unsigned int>();
	}

	if (vm.count("buffers")){
		buffers = vm["buffers"].as<unsigned int>();
	}

	if (vm.count("iterations")){
		iterations = vm["iterations"].as<unsigned int>();
	}

	if (vm.count("tests")){
		tests = vm["tests"].as<unsigned int>();
	}

	if (vm.count("threads")){
		threads = vm["threads"].as<unsigned int>();
	}

	BufferBuilder bb("/dls/detectors/Timepix3/I16_20160422/raw_data/W2J2_top/1hour", packetsPerBuffer);
	bb.build(buffers);
	bb.setNumIterations(iterations);


	tbb::task_scheduler_init init_parallel;
    double tTime = run_pipeline(&bb, threads);
	double tData = (double)(bb.getBufferWordCount())/1024.0/1024.0*(double)(buffers*iterations*sizeof(u_int64_t));
	printf("Data processed: %f MB\n", tData);
	printf("Time taken %.6f s\n", tTime);
	printf("Processing rate %.6f MB/s\n", tData/tTime);

	return 0;
}


