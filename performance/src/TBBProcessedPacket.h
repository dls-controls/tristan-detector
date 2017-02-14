/*
 * TBBProcessedPacket.h
 *
 *  Created on: 13 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_TBBPROCESSEDPACKET_H_
#define SRC_TBBPROCESSEDPACKET_H_

#include <tbb/tbb.h>

class TBBProcessedPacket
{
	//! Pointer to one past last character in sequence
	size_t max_size_;
	u_int32_t* events_;
	u_int64_t* timestamps_;
	u_int64_t* energies_;
	int index_;

public:

    static TBBProcessedPacket* allocate(size_t max_size)
    {
    	TBBProcessedPacket* p = (TBBProcessedPacket *)tbb::tbb_allocator<char>().allocate(sizeof(TBBProcessedPacket));
    	p->max_size_ = max_size;
    	p->events_ = (u_int32_t *)tbb::tbb_allocator<u_int32_t>().allocate(sizeof(u_int32_t)*max_size);
    	p->timestamps_ = (u_int64_t *)tbb::tbb_allocator<u_int64_t>().allocate(sizeof(u_int64_t)*max_size);
    	p->energies_ = (u_int64_t *)tbb::tbb_allocator<u_int64_t>().allocate(sizeof(u_int64_t)*max_size);
        p->index_ = 0;
    	return p;
    }

    //! Free a TextSlice object
    void free() {
        tbb::tbb_allocator<u_int32_t>().deallocate((u_int32_t*)this->events_,sizeof(u_int32_t)*this->max_size_);
        tbb::tbb_allocator<u_int64_t>().deallocate((u_int64_t*)this->timestamps_,sizeof(u_int64_t)*this->max_size_);
        tbb::tbb_allocator<u_int64_t>().deallocate((u_int64_t*)this->energies_,sizeof(u_int64_t)*this->max_size_);
        tbb::tbb_allocator<char>().deallocate((char*)this,sizeof(TBBProcessedPacket));
    }

    //! Append sequence [first,last) to this sequence.
    void append( u_int32_t event, u_int64_t timestamp, u_int64_t energy)
    {
    	this->events_[this->index_] = event;
    	this->timestamps_[this->index_] = timestamp;
    	this->energies_[this->index_] = energy;
    	this->index_++;
    }

    void print(int index)
    {
    	printf("Event[%d] ID[%u] time[%lu] energy[%lu]\n", index, this->events_[index], this->timestamps_[index], this->energies_[index]);
    }
};

#endif /* SRC_TBBPROCESSEDPACKET_H_ */
