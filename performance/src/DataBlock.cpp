/*
 * DataBlock.cpp
 *
 *  Created on: 16 Feb 2017
 *      Author: gnx91527
 */

#include "DataBlock.h"

DataBlock::DataBlock(int id, int maxSize)
{
	ID_ = id;
	maxSize_ = maxSize;
	index_ = 0;
	eventID_ = (u_int32_t *)malloc(maxSize_ * sizeof(u_int32_t));
	energy_ = (u_int32_t *)malloc(maxSize_ * sizeof(u_int32_t));
	timestamp_ = (u_int64_t *)malloc(maxSize_ * sizeof(u_int64_t));
}

DataBlock::~DataBlock()
{
	//printf("Destroying block: %d\n", ID_);
	if (eventID_){
		free(eventID_);
	}
	if (energy_){
		free(energy_);
	}
	if (timestamp_){
		free(timestamp_);
	}
}

void DataBlock::addItem(u_int32_t energy, u_int64_t timestamp, u_int32_t eventID)
{
	energy_[index_] = energy;
	eventID_[index_] = eventID;
	timestamp_[index_] = timestamp;
	index_++;
}

void DataBlock::copy(DataBlock *bPtr, int start, int size)
{
	u_int32_t *evPtr1 = &(eventID_[index_]);
	u_int32_t *evPtr2 = &(bPtr->eventID_[start]);
	u_int32_t *enPtr1 = &(energy_[index_]);
	u_int32_t *enPtr2 = &(bPtr->energy_[start]);
	u_int64_t *tsPtr1 = &(timestamp_[index_]);
	u_int64_t *tsPtr2 = &(bPtr->timestamp_[start]);
	memcpy(evPtr1, evPtr2, size*sizeof(u_int32_t));
	memcpy(enPtr1, enPtr2, size*sizeof(u_int32_t));
	memcpy(tsPtr1, tsPtr2, size*sizeof(u_int64_t));
	index_ += size;
}

int DataBlock::freeCapacity()
{
	return maxSize_ - index_;
}
