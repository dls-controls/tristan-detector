/*
 * DataBlock.h
 *
 *  Created on: 16 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_DATABLOCK_H_
#define SRC_DATABLOCK_H_

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <cstring>

class DataBlock {
public:
	DataBlock(int id, int maxSize);
	virtual ~DataBlock();

	void addItem(u_int32_t energy, u_int64_t timestamp, u_int32_t eventID);
	void copy(DataBlock *bPtr, int start, int size);
	int freeCapacity();

	int ID_;
	int maxSize_;
	int index_;
	u_int32_t *energy_;
	u_int64_t *timestamp_;
	u_int32_t *eventID_;
};

#endif /* SRC_DATABLOCK_H_ */
