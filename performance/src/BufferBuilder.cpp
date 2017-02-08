/*
 * BufferBuilder.cpp
 *
 *  Created on: 7 Feb 2017
 *      Author: gnx91527
 */

#include "BufferBuilder.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

BufferBuilder::BufferBuilder(const std::string& dataPath, int packetsPerBuffer) :
	qtyBuffers_(0),
	wordsPerPacket_(1022),
	packetsPerBuffer_(packetsPerBuffer)
{
	asr_ = new AsciiStackReader(dataPath);
	// Throw away some initial samples to get into the data collection proper
	for (int count = 0; count < 50000; count++){
		asr_->readNext();
	}
}

BufferBuilder::~BufferBuilder()
{
	delete asr_;
}

int BufferBuilder::build(int qtyBuffers)
{
	qtyBuffers_ = qtyBuffers;
	u_int64_t tsCourse = 0;

	while (tsCourse == 0){
		DataWord word(asr_->readNext());
		if (word.isCtrl()){
	    	if (word.ctrlType() == 0x20){
		    	printf("Found initial course timestamp packet\n");
	    		// This is a timestamp packet
	    		tsCourse = word.getRaw();
	    	}
		}
	}

	for (int cBuff = 0; cBuff < qtyBuffers_; cBuff++){
		printf("Constructing buffer number %d out of %d\n", cBuff+1, qtyBuffers_);
		void *bufferPtr = malloc(sizeof(u_int64_t) * wordsPerPacket_ * packetsPerBuffer_);
		for (int bIndex = 0; bIndex < packetsPerBuffer_; bIndex++){
			// Now build a qty of packets, each with 1022 words
			int words = 0;
			u_int64_t packet[wordsPerPacket_];
			packet[words] = tsCourse;
			words++;
			while (words < wordsPerPacket_){
				DataWord word(asr_->readNext());
				packet[words] = word.getRaw();
				words++;
				if (word.isCtrl()){
					if (word.ctrlType() == 0x20){
						tsCourse = word.getRaw();
					}
				}
			}
			memcpy((char *)bufferPtr+(bIndex*wordsPerPacket_*sizeof(u_int64_t)), (void *)packet, wordsPerPacket_*sizeof(u_int64_t));
		}
		buffers_.push_back(bufferPtr);
	}
}

void *BufferBuilder::getBufferPtr(int index)
{
	void *bPtr = NULL;
	// Verify the index
	if (index < 0 || index >= qtyBuffers_){
		printf("Incorrect index provided\n");
	} else {
		bPtr = buffers_[index];
	}
	return bPtr;
}

int BufferBuilder::getBufferWordCount()
{
	return wordsPerPacket_ * packetsPerBuffer_;
}
