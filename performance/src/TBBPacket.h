/*
 * TBBPacket.h
 *
 *  Created on: 13 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_TBBPACKET_H_
#define SRC_TBBPACKET_H_

#include <tbb/tbb.h>

class TBBPacket
{
public:

	//! Pointer to one past last character in sequence
	size_t max_size_;
	u_int64_t* beginning_;
	u_int64_t* end_;
	u_int32_t* events_;
	u_int64_t* timestamps_;
	u_int64_t* energies_;
	int index_;


    static TBBPacket* allocate(size_t max_size)
    {
    	//printf("Allocating with size: %d\n", max_size);
    	TBBPacket* p = (TBBPacket *)tbb::tbb_allocator<char>().allocate(sizeof(TBBPacket));
    	p->max_size_ = max_size;
    	p->beginning_ = (u_int64_t *)tbb::tbb_allocator<u_int64_t>().allocate(sizeof(u_int64_t)*max_size);
        p->end_ = p->begin()+max_size;
    	p->events_ = (u_int32_t *)tbb::tbb_allocator<u_int32_t>().allocate(sizeof(u_int32_t)*max_size);
    	p->timestamps_ = (u_int64_t *)tbb::tbb_allocator<u_int64_t>().allocate(sizeof(u_int64_t)*max_size);
    	p->energies_ = (u_int64_t *)tbb::tbb_allocator<u_int64_t>().allocate(sizeof(u_int64_t)*max_size);
        p->index_ = 0;
    	return p;
    }

    //! Free a TextSlice object
    void free() {
        tbb::tbb_allocator<u_int64_t>().deallocate((u_int64_t*)this->beginning_,sizeof(u_int64_t)*this->max_size_);
        tbb::tbb_allocator<u_int32_t>().deallocate((u_int32_t*)this->events_,sizeof(u_int32_t)*this->max_size_);
        tbb::tbb_allocator<u_int64_t>().deallocate((u_int64_t*)this->timestamps_,sizeof(u_int64_t)*this->max_size_);
        tbb::tbb_allocator<u_int64_t>().deallocate((u_int64_t*)this->energies_,sizeof(u_int64_t)*this->max_size_);
        tbb::tbb_allocator<char>().deallocate((char*)this,sizeof(TBBPacket));
    }

    //! Pointer to beginning of sequence
    u_int64_t* begin() {return this->beginning_;}

    //! Pointer to one past last character in sequence
    u_int64_t* end() {return this->end_;}

    //! Length of sequence
    size_t size() const {return this->max_size_;}

    //! Maximum number of characters that can be appended to sequence
    size_t avail() const {return this->max_size_;}

    //! Append sequence [first,last) to this sequence.
    void append( u_int64_t* first, size_t num)
    {
        memcpy(this->beginning_, first, sizeof(u_int64_t)*num);
    }

    //! Set end() to given value.
    void set_end( u_int64_t* p ) {this->end_=p;}

    //! Append sequence [first,last) to this sequence.
    void setValue( u_int32_t event, u_int64_t timestamp, u_int64_t energy)
    {
    	this->events_[this->index_] = event;
    	this->timestamps_[this->index_] = timestamp;
    	this->energies_[this->index_] = energy;
    	//if (this->index_ >= 1020){
    		//printf("Address of this: %ul\n", this);
    		//printf("**Set Val[%d] ID[%u] time[%lu] energy[%lu]\n", this->index_, this->events_[this->index_], this->timestamps_[this->index_], this->energies_[this->index_]);
    	//}
    	this->index_++;
    }

    void report()
    {
    	printf("*** Max index: %d\n", this->index_);
    }

};

#endif /* SRC_TBBPACKET_H_ */
