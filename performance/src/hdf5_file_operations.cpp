/*
 * hdf5_file_operations.cpp
 *
 *  Created on: 4 Apr 2017
 *      Author: gnx91527
 */

#include "hdf5_file_operations.h"
#include <assert.h>
#include <vector>
#include <iostream>
#include <boost/program_options.hpp>

hid_t createFile(const std::string& filename, hsize_t alignment)
{
    hid_t fapl; // File access property list
    hid_t fcpl;
    hid_t fileID;

    // Create file access property list
    fapl = H5Pcreate(H5P_FILE_ACCESS);

    H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG);

    // Set chunk boundary alignment (default 4MB)
    H5Pset_alignment(fapl, 65536, alignment);

    // Set to use the latest library format
    H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST);

    // Create file creation property list
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)

    // Creating the file with SWMR write access
    printf("Creating file: %s\n", filename.c_str());
    unsigned int flags = H5F_ACC_TRUNC;
    fileID = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
    if (fileID < 0){
      // Close file access property list
      H5Pclose(fapl);
      // Now throw a runtime error to explain that the file could not be created
      std::stringstream err;
      err << "Could not create file " << filename;
      throw std::runtime_error(err.str().c_str());
    }
    // Close file access property list
    H5Pclose(fapl);
    H5Pclose(fcpl);

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
      H5Pclose(prop);
      H5Pclose(dapl);
      H5Sclose(dataspace);
      // Now throw a runtime error to notify that the dataset could not be created
      throw std::runtime_error("Unable to create the dataset");
    }

    H5Pclose(prop);
    H5Pclose(dapl);
    H5Sclose(dataspace);

    return dataset;
}

void extendDataset(hid_t dsetID, hsize_t dsetSize, hsize_t extraSize)
{
	herr_t status;
	hsize_t newSize = dsetSize + extraSize;
    status = H5Dset_extent(dsetID, &newSize);
    assert(status >= 0);
}

void writeDataset(hid_t dsetID, hsize_t dsetSize, hsize_t chunkSize, void *dPtr, int direct, int flush)
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
		H5Dflush(dsetID);
	}

}

void closeDataset(hid_t dsetID)
{
	if (dsetID >= 0) {
//	    printf("Closing dataset\n");
        H5Dclose(dsetID);
        dsetID = -1;
    }
}

void closeFile(hid_t fileID)
{
	if (fileID >= 0) {
//	    printf("Closing file\n");
        H5Fclose(fileID);
        fileID = -1;
    }
}



