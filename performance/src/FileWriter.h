/*
 * FileWriter.h
 *
 *  Created on: 15 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_FILEWRITER_H_
#define SRC_FILEWRITER_H_

#include "DataBlock.h"
#include "TBBPacket.h"
#include <assert.h>
#include <string>
#include <hdf5_hl.h>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <math.h>

typedef struct Dataset
{
	hid_t id_;
	hsize_t dim_;
} Dataset;

class FileWriter
{
public:
	FileWriter(hsize_t chunkSize=(4*1024*2014), hsize_t alignment=(4*1024*2014));
	virtual ~FileWriter();

	void startWriting(const std::string& filename);
	void createFile(const std::string& filename);
	hid_t createDataset(const std::string& name, hid_t dtype);
	void extendDataset(Dataset &dset, size_t bufferSize) const;
	void addBuffer(DataBlock *bPtr);
	void addTBBBuffer(TBBPacket *bPtr);
	void writeDataset(Dataset &dset, hid_t dataType, int size, void *dPtr);
	void closeFile();

private:
	hid_t fileID_;
	hsize_t chunkSize_;
	hsize_t alignment_;
	Dataset energy_;
	Dataset timestamp_;
	Dataset eventID_;
};

#endif /* SRC_FILEWRITER_H_ */
