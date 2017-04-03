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

namespace opt = boost::program_options;

hid_t createFile(const std::string& filename, hsize_t alignment)
{
    hid_t fapl; // File access property list
    hid_t fcpl;
    hid_t fileID;

    // Create file access property list
    fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);

    assert(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) >= 0);

    // Set chunk boundary alignment (default 4MB)
    assert( H5Pset_alignment(fapl, 65536, alignment) >= 0);

    // Set to use the latest library format
    assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);

    // Create file creation property list
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
    assert(fcpl >= 0);

    // Creating the file with SWMR write access
    printf("Creating file: %s\n", filename.c_str());
    unsigned int flags = H5F_ACC_TRUNC;
    fileID = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
    if (fileID < 0){
      // Close file access property list
      assert(H5Pclose(fapl) >= 0);
      // Now throw a runtime error to explain that the file could not be created
      std::stringstream err;
      err << "Could not create file " << filename;
      throw std::runtime_error(err.str().c_str());
    }
    // Close file access property list
    assert(H5Pclose(fapl) >= 0);
    assert(H5Pclose(fcpl) >= 0);

    return fileID;
}

hid_t createDataset(hid_t fileID, const std::string& name, hid_t dtype, hsize_t chunkSize, int niter)
{
    // Handles all at the top so we can remember to close them
    hid_t dataset = 0;
    hid_t dataspace = 0;
    hid_t prop = 0;
    hid_t dapl = 0;
    herr_t status;

    // Dataset dims: {1, <image size Y>, <image size X>}
    std::vector<hsize_t> dset_dims(1,1);

    hsize_t chunkdim = 1;
    // Set the chunk size according to the dataset type
    if (dtype == H5T_NATIVE_UINT32){
    	chunkdim = chunkSize / 4;
    }
    if (dtype == H5T_NATIVE_UINT64){
    	chunkdim = chunkSize / 8;
    }
    std::vector<hsize_t> chunk_dims(1, chunkdim);

    std::vector<hsize_t> max_dims = dset_dims;
    max_dims[0] = H5S_UNLIMITED;

    // Create the dataspace with the given dimensions - and max dimensions
    dataspace = H5Screate_simple(dset_dims.size(), &dset_dims.front(), &max_dims.front());
    assert(dataspace >= 0);

    // Enable chunking
    prop = H5Pcreate(H5P_DATASET_CREATE);
    status = H5Pset_chunk(prop, dset_dims.size(), &chunk_dims.front());
    assert(status >= 0);

    char fill_value[8] = {0,0,0,0,0,0,0,0};
    status = H5Pset_fill_value(prop, dtype, fill_value);
    assert(status >= 0);

    dapl = H5Pcreate(H5P_DATASET_ACCESS);
    //size_t nbytes = sizeof(uint32_t) * chunk_dims[0];
    //size_t nslots = static_cast<size_t>(ceil((double)max_dims[0] / chunk_dims[0]) * niter);
    //size_t nslots = static_cast<size_t>(ceil((double)chunk_dims[0]) * niter);
    //nslots *= 13;
    //assert( H5Pset_chunk_cache( dapl, nslots, nbytes, 1.0) >= 0);
    //assert( H5Pset_chunk_cache( dapl, 0, chunkSize, 1.0) >= 0);

    dataset = H5Dcreate2(fileID,
    					 name.c_str(),
                         dtype,
						 dataspace,
						 H5P_DEFAULT,
						 prop,
						 dapl);

    if (dataset < 0){
      // Unable to create the dataset, clean up resources
      assert( H5Pclose(prop) >= 0);
      assert( H5Pclose(dapl) >= 0);
      assert( H5Sclose(dataspace) >= 0);
      // Now throw a runtime error to notify that the dataset could not be created
      throw std::runtime_error("Unable to create the dataset");
    }

    assert( H5Pclose(prop) >= 0);
    assert( H5Pclose(dapl) >= 0);
    assert( H5Sclose(dataspace) >= 0);

    return dataset;
}

void extendDataset(hid_t dsetID, hsize_t dsetSize, hsize_t extraSize)
{
	herr_t status;
	hsize_t newSize = dsetSize + extraSize;
    status = H5Dset_extent(dsetID, &newSize);
    assert(status >= 0);
}

void writeDataset(hid_t dsetID, hsize_t dsetSize, hsize_t chunkSize, void *dPtr, int direct = 1, int flush=0)
{
	herr_t status;
    uint32_t filter_mask = 0x0;
	hsize_t offsets[1] = {dsetSize};
	hsize_t img_dims[1] = {dsetSize+chunkSize};
	hsize_t max_dims[1] = {H5S_UNLIMITED};
	hsize_t dspace_dims[1] = {chunkSize};

	if (direct){
		status = H5DOwrite_chunk(dsetID,
				H5P_DEFAULT,
				filter_mask,
				offsets,
				chunkSize*sizeof(uint32_t),
				dPtr);
	} else {
		// Select a hyperslab
		hid_t filespace = H5Dget_space(dsetID);
		assert(filespace >= 0);
		status = H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, NULL,
									 dspace_dims, NULL);
		assert(status >= 0);
	    hid_t dataspace = H5Screate_simple(1, dspace_dims, dspace_dims);

		status = H5Dwrite(dsetID,
				H5T_NATIVE_UINT32,
				dataspace,
				filespace,
				H5P_DEFAULT,
				dPtr);
		H5Sclose(dataspace);
		assert(status >= 0);
	}

	if (flush){
		assert(H5Dflush(dsetID) >= 0);
	}

}

void closeFile(hid_t fileID)
{
	if (fileID >= 0) {
	    printf("Closing file\n");
        assert(H5Fclose(fileID) >= 0);
        fileID = -1;
    }
}

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
			("gpfs,g", opt::value<unsigned int>()->default_value(1), "Use GPFS0(1) or gpfs0(2)")
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
		if (vm["gpfs"].as<unsigned int>() == 2){
			fname << "/mnt/gpfs02/testdir/gnx91527/latrd/latrd_" << fnum << ".h5";
		} else {
			fname << "/mnt/GPFS01/tmp/gnx91527/latrd/latrd_" << fnum << ".h5";
		}
	} else {
		fname << "/mnt/GPFS01/tmp/gnx91527/latrd/latrd_" << fnum << ".h5";
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

	// Allocate the buffer large enough to hold 1 chunk worth of data
	int buffSize = chunkSize / sizeof(uint32_t);
	uint32_t buffer[buffSize];
	memset(buffer, buffSize, 1);

	double times[iterations];

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

		writeDataset(dsetID, dsetSize, buffSize, buffer);
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

	closeFile(fileID);

	printf("Processing rate %.6f MB/s\n", double(chunkSize*iterations) / (1024*1024) / tTime);

	FILE *f = fopen("/home/gnx91527/performance.log", "w");
	for (int loop=0; loop < iterations; loop++){
		fprintf(f, "%d %f\n", loop, times[loop]);
	}
	fclose(f);
	return 0;
}






