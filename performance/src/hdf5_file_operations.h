/*
 * hdf5_file_operations.h
 *
 *  Created on: 4 Apr 2017
 *      Author: gnx91527
 */

#ifndef PERFORMANCE_SRC_HDF5_FILE_OPERATIONS_H_
#define PERFORMANCE_SRC_HDF5_FILE_OPERATIONS_H_

#include <string>
#include <hdf5_hl.h>
#include <iostream>

hid_t createFile(const std::string& filename, hsize_t alignment);
hid_t createDataset(hid_t fileID, const std::string& name, hid_t dtype, hsize_t chunkSize, int niter);
void extendDataset(hid_t dsetID, hsize_t dsetSize, hsize_t extraSize);
void writeDataset(hid_t dsetID, hsize_t dsetSize, hsize_t chunkSize, void *dPtr, int direct = 1, int flush=0);
void closeDataset(hid_t dsetID);
void closeFile(hid_t fileID);


#endif /* PERFORMANCE_SRC_HDF5_FILE_OPERATIONS_H_ */
