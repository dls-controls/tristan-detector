/*
 * standard_performance.cpp
 *
 *  Created on: 6 Feb 2017
 *      Author: gnx91527
 */

#include "BufferBuilder.h"
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
#include <boost/bind.hpp>
#include <boost/thread.hpp>

namespace opt = boost::program_options;



class thread_pool
{
private:
  std::queue< boost::function< void() > > tasks_;
  boost::thread_group threads_;
  std::size_t available_;
  boost::mutex mutex_;
  boost::condition_variable condition_;
  bool running_;
  std::size_t size_;
public:

  /// @brief Constructor.
  thread_pool( std::size_t pool_size )
    : available_( pool_size ),
      running_( true ),
	  size_(pool_size)
  {
    for ( std::size_t i = 0; i < pool_size; ++i )
    {
      threads_.create_thread( boost::bind( &thread_pool::pool_main, this ) ) ;
    }
  }

  /// @brief Destructor.
  ~thread_pool()
  {
    // Set running flag to false then notify all threads.
    {
      boost::unique_lock< boost::mutex > lock( mutex_ );
      running_ = false;
      condition_.notify_all();
    }

    try
    {
      threads_.join_all();
    }
    // Suppress all exceptions.
    catch ( const std::exception& ) {}
  }

  /// @brief Add task to the thread pool if a thread is currently available.
  template < typename Task >
  void run_task( Task task )
  {
    boost::unique_lock< boost::mutex > lock( mutex_ );

    // If no threads are available, then wait
    while ( 0 == available_ ){
    	lock.unlock();
    	boost::thread::yield();
    	lock.lock();
    }

    // Decrement count, indicating thread is no longer available.
    --available_;

    // Set task and signal condition variable so that a worker thread will
    // wake up andl use the task.
    tasks_.push( boost::function< void() >( task ) );
    condition_.notify_one();
  }

  void waitForIdle()
  {
	    boost::unique_lock< boost::mutex > lock( mutex_ );
printf("Available : %d\n", available_);
	    while (available_ < size_){
	    	lock.unlock();
	    	boost::thread::yield();
	    	lock.lock();
	    }
	    printf("Available after: %d\n", available_);
  }

private:
  /// @brief Entry point for pool threads.
  void pool_main()
  {
    while( running_ )
    {
      // Wait on condition variable while the task is empty and the pool is
      // still running.
      boost::unique_lock< boost::mutex > lock( mutex_ );
      while ( tasks_.empty() && running_ )
      {
        condition_.wait( lock );
      }
      // If pool is no longer running, break out.
      if ( !running_ ) break;

      // Copy task locally and remove from the queue.  This is done within
      // its own scope so that the task object is destructed immediately
      // after running the task.  This is useful in the event that the
      // function contains shared_ptr arguments bound via bind.
      {
        boost::function< void() > task = tasks_.front();
        tasks_.pop();

        lock.unlock();

        // Run the task.
        try
        {
          task();
        }
        // Suppress all exceptions.
        catch ( const std::exception& ) {}
      }

      // Task has finished, so increment count of available threads.
      lock.lock();
      ++available_;
    } // while running_
  }
};

int findMatchTs(u_int64_t timeStamp)
{
	// This method will return the 2 bits required for
	// matching a course timestamp to an extended timestamp
	return (timeStamp >> 22) & 0x03;
}

u_int64_t fullTimestamp(int mc, int mpc, u_int64_t prevTsCourse, u_int64_t currTsCourse, u_int64_t tsFine)
{
	u_int64_t fullTs = 0;
	int mf = (tsFine >> 22) & 0x03;
	if ((mc == mf) || (mc+1 == mf)){
		fullTs = (currTsCourse & 0x0FFFFFFF000000) + tsFine;
	} else {
		if ((mpc == mf) || (mpc+1 == mf)){
			fullTs = (prevTsCourse & 0x0FFFFFFF000000) + tsFine;
		} else {
			//printf("********* Warning ***********\n");
			//printf("findMatchTs(currTsCourse): %d\n", findMatchTs(currTsCourse));
			//printf("findMatchTs(prevTsCourse): %d\n", findMatchTs(prevTsCourse));
			//printf("findMatchTs(tsFine): %d\n", findMatchTs(tsFine));
		}
	}
	return fullTs;
}

static u_int64_t eventCount = 0;
static u_int64_t ctrlCount = 0;
static u_int64_t csTimeCount = 0;
static boost::mutex mutex;

void recordEvent(u_int64_t events, u_int64_t ctrl, u_int64_t time)
{
	boost::unique_lock< boost::mutex > lock(mutex);
	eventCount+=events;
	ctrlCount+=ctrl;
	csTimeCount+=time;
}

void processBuffer(u_int64_t *bPtr, int bufferWordCount)
{
	u_int64_t csTime = 0;
	u_int64_t csPrevTime = 0;
	u_int32_t eventID = 0;
	u_int64_t timeStamp = 0;
	u_int64_t energy = 0;
	u_int64_t tsFine = 0;
	int mf = 0;
	int mc = 0;
	int mpc = 0;
	u_int64_t word = 0;
	u_int64_t localEventCount = 0;
	u_int64_t localCtrlCount = 0;
	u_int64_t localTimeCount = 0;
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
			eventID = ((word >> 39) & 0x0000000000FFFFFF);
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
///////////////////////////////////
			energy = word & 0x0000000000003FFF;
		}
	}
	//printf("processBuffer exit\n");
	recordEvent(localEventCount, localCtrlCount, localTimeCount);
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
			("packets,p", opt::value<unsigned int>()->default_value(512), "Number of packets per buffer")
			("buffers,b", opt::value<unsigned int>()->default_value(10), "Number of buffers to create")
			("iterations,i", opt::value<unsigned int>()->default_value(10), "Number of iterations per test")
			("tests,t", opt::value<unsigned int>()->default_value(20), "Number of times to execute test")
			("threads,s", opt::value<unsigned int>()->default_value(1), "Number of threads to spawn");

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


	thread_pool tp(threads);

	double rates[tests];
	for (int testNumber = 0; testNumber < tests; testNumber++){
		timeval curTime;
		gettimeofday(&curTime, NULL);
		unsigned long micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
		printf("Test number %d\n", testNumber+1);
		printf("===============\n");

		u_int64_t csTime = 0;
		u_int64_t csPrevTime = 0;
		u_int32_t eventID = 0;
		u_int64_t timeStamp = 0;
		u_int64_t energy = 0;
		u_int64_t tsFine = 0;
		int mf = 0;
		int mc = 0;
		int mpc = 0;

		eventCount = 0;
		ctrlCount = 0;
		csTimeCount = 0;

		// Loop over the words in the buffer
		u_int64_t word = 0;
		int bufferWordCount = bb.getBufferWordCount();

		for (int loop=0; loop < iterations; loop++){
			// Get a buffer
			u_int64_t *bPtr = (u_int64_t *)bb.getBufferPtr(loop%buffers);

			/*
			 * This will assign tasks to the thread pool.
			 * More about boost::bind: "http://www.boost.org/doc/libs/1_54_0/libs/bind/bind.html#with_functions"
			 */
			tp.run_task(boost::bind(processBuffer, bPtr, bufferWordCount));

		}

		tp.waitForIdle();

		gettimeofday(&curTime, NULL);
		micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
		double tTime = (double)micro / 1000000.0;
		double tData = (double)(bufferWordCount)/1024.0/1024.0*(double)(iterations*sizeof(u_int64_t));
		printf("Control words: %lu\n", ctrlCount);
		printf("Course time words: %lu\n", csTimeCount);
		printf("Event words: %lu\n", eventCount);
		printf("Data processed: %f MB\n", tData);
		printf("Time taken %.6f s\n", tTime);
		printf("Processing rate %.6f MB/s\n", tData/tTime);
		rates[testNumber] = tData/tTime;
	}

	double averageRate = 0.0;
	double sdRate = 0.0;
	for (int index = 0; index < tests; index++){
		averageRate += rates[index];
	}
	averageRate = averageRate/(double)tests;
	for (int index = 0; index < tests; index++){
		sdRate += (rates[index] - averageRate) * (rates[index] - averageRate);
	}
	sdRate = sqrt(sdRate/(double)tests);
	printf("Mean processing rate %.6f MB/s\n", averageRate);
	printf("SD processing rate %.6f MB/s\n", sdRate);



	return 0;
}


