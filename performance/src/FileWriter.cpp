/*
 * FileWriter.cpp
 *
 *  Created on: 15 Feb 2017
 *      Author: gnx91527
 */

#include "FileWriter.h"
#include <sys/time.h>

FileWriter::FileWriter(hsize_t chunkSize, hsize_t alignment) :
	fileID_(-1),
	chunkSize_(chunkSize),
	alignment_(alignment)
{

}

FileWriter::~FileWriter()
{

}

void FileWriter::startWriting(const std::string& filename)
{
	this->createFile(filename);
	this->eventID_.id_ = this->createDataset("event", H5T_NATIVE_UINT32);
	this->eventID_.dim_ = 0;
	this->timestamp_.id_ = this->createDataset("timestamp", H5T_NATIVE_UINT64);
	this->timestamp_.dim_ = 0;
	this->energy_.id_ = this->createDataset("energy", H5T_NATIVE_UINT32);
	this->energy_.dim_ = 0;
}

void FileWriter::createFile(const std::string& filename)
{
    hid_t fapl; // File access property list
    hid_t fcpl;

    // Create file access property list
    fapl = H5Pcreate(H5P_FILE_ACCESS);
    assert(fapl >= 0);

    assert(H5Pset_fclose_degree(fapl, H5F_CLOSE_STRONG) >= 0);

    // Set chunk boundary alignment (default 4MB)
    assert( H5Pset_alignment(fapl, 65536, alignment_) >= 0);

    // Set to use the latest library format
    //assert(H5Pset_libver_bounds(fapl, H5F_LIBVER_LATEST, H5F_LIBVER_LATEST) >= 0);

    // Create file creation property list
    if ((fcpl = H5Pcreate(H5P_FILE_CREATE)) < 0)
    assert(fcpl >= 0);

    // Creating the file with SWMR write access
    printf("Creating file: %s\n", filename.c_str());
    unsigned int flags = H5F_ACC_TRUNC;
    this->fileID_ = H5Fcreate(filename.c_str(), flags, fcpl, fapl);
    if (this->fileID_ < 0){
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
}

hid_t FileWriter::createDataset(const std::string& name, hid_t dtype)
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
    	chunkdim = chunkSize_ / 4;
    }
    if (dtype == H5T_NATIVE_UINT64){
    	chunkdim = chunkSize_ / 8;
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
    //size_t nslots = static_cast<size_t>(ceil((double)max_dims[1] / chunk_dims[1]));
    //nslots *= 13;
    //assert( H5Pset_chunk_cache( dapl, nslots, nbytes, 1.0) >= 0);

    dataset = H5Dcreate2(this->fileID_,
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

void FileWriter::extendDataset(Dataset &dset, size_t bufferSize) const {
	herr_t status;
    dset.dim_ += bufferSize;
    status = H5Dset_extent( dset.id_, &(dset.dim_));
    assert(status >= 0);
}

void FileWriter::addBuffer(DataBlock *bPtr)
{
	//printf("FileWriter::addBuffer Energy[65537]: %d\n", bPtr->energy_[65537]);
	//printf("FileWriter::addBuffer bPtr->index_: %d\n", bPtr->index_);
	this->writeDataset(eventID_, H5T_NATIVE_UINT32, bPtr->index_, bPtr->eventID_);
//	this->writeDataset(energy_, H5T_NATIVE_UINT32, bPtr->index_, bPtr->energy_);
//	this->writeDataset(timestamp_, H5T_NATIVE_UINT64, bPtr->index_, bPtr->timestamp_);
}

void FileWriter::addTBBBuffer(TBBPacket *bPtr)
{
	this->writeDataset(eventID_, H5T_NATIVE_UINT32, bPtr->index_, bPtr->events_);
	this->writeDataset(energy_, H5T_NATIVE_UINT32, bPtr->index_, bPtr->energies_);
	this->writeDataset(timestamp_, H5T_NATIVE_UINT64, bPtr->index_, bPtr->timestamps_);
}

void FileWriter::writeDataset(Dataset &dset, hid_t dataType, int size, void *dPtr)
{
	herr_t status;
	// Find out how large the buffer is
	// Extend the dataset
	// Write out the buffer

    hsize_t chunkdim = 1;
    // Set the chunk size according to the dataset type
    if (dataType == H5T_NATIVE_UINT32){
    	chunkdim = chunkSize_ / 4;
    }
    if (dataType == H5T_NATIVE_UINT64){
    	chunkdim = chunkSize_ / 8;
    }
//printf("Size: %d\n", size);
//printf("Chunkdim: %d\n", chunkdim);

    // Set the offset
    hsize_t offsetVal = dset.dim_;

    int calcSize = (size/chunkdim) * chunkdim;
    //printf("Calculated Size: %d\n", calcSize);
	this->extendDataset(dset, calcSize);

//    hid_t filespace = H5Dget_space(dset.id_);
//    hsize_t dataSize = (hsize_t)size;
    //printf("dataSize: %d\n", dataSize);
    // Select the hyperslab
//    H5Sselect_hyperslab(filespace, H5S_SELECT_SET, offsets, NULL, &dataSize, NULL);

//    hid_t memspace = H5Screate_simple(1, &dataSize, NULL);

//    uint32_t filter_mask = 0x0;
//    status = H5Dwrite(dset.id_, dataType,
//                             memspace, filespace,
//                             H5P_DEFAULT, dPtr);

    uint32_t filter_mask = 0x0;
    //uint32_t *valPtr = (uint32_t *)dPtr;

    //printf("dset.dim_: %d\n", dset.dim_);
	hsize_t offsets[1]; // = {offsetVal};
    while ((offsetVal+chunkdim) <= dset.dim_){
		timeval curTime;
		gettimeofday(&curTime, NULL);
		unsigned long micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec;
    	//printf("Offset Val: %d\n", offsetVal);
		offsets[0] = offsetVal;
    	status = H5DOwrite_chunk(dset.id_, H5P_DEFAULT,
    			filter_mask, offsets,
				chunkSize_, dPtr);
    	offsetVal += calcSize;
		gettimeofday(&curTime, NULL);
		micro = curTime.tv_sec*(u_int64_t)1000000+curTime.tv_usec - micro;
		double tTime = (double)micro / 1000000.0;
		if (tTime > 0.002){
			printf("Chunk write time: %f\n", tTime);
			printf("  DSET ID: %d\n", dset.id_);
			printf("  Offset: %d\n", offsets[0]);
			printf("  CalcSize: %d\n", calcSize);
		}
    	//valPtr += chunkdim;
    }

}

void FileWriter::closeFile()
{
/*	if (this->dsetEnergy_ >= 0){
		assert(H5Dclose(this->dsetEnergy_));
		this->dsetEnergy_ = -1;
	}
	if (this->dsetEventID_ >= 0){
		assert(H5Dclose(this->dsetEventID_));
		this->dsetEventID_ = -1;
	}
	if (this->dsetTimestamp_ >= 0){
		assert(H5Dclose(this->dsetTimestamp_));
		this->dsetTimestamp_ = -1;
	}*/
	if (this->fileID_ >= 0) {
	    printf("Closing file\n");
        assert(H5Fclose(this->fileID_) >= 0);
        this->fileID_ = -1;
    }
}
