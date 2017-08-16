/*
 * LATRDBuffer.cpp
 *
 *  Created on: 15 Aug 2017
 *      Author: gnx91527
 */

#include "LATRDBuffer.h"

namespace FrameProcessor {

LATRDBuffer::LATRDBuffer(size_t numberOfDataPoints, const std::string& frame) :
		numberOfPoints_(numberOfDataPoints),
		currentPoint_(0),
		frameName_(frame)
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.LATRDBuffer");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "LATRDBuffer constructor.");

    // Allocate the memory block required for the points
    LOG4CXX_DEBUG(logger_, "Number of data points [" << numberOfPoints_ << "]");
    size_t bytes_to_allocate = numberOfPoints_ * bytes_per_data_point;
    LOG4CXX_DEBUG(logger_, "Total bytes to allocate [" << bytes_to_allocate << "]");
	rawDataPtr_ = malloc(bytes_to_allocate);
    LOG4CXX_DEBUG(logger_, "Allocated memory block with base address [" << std::hex << rawDataPtr_ << "]");
	// Point the timestamp pointer to the start of the block
	eventTimestampPtr_ = (uint64_t *)rawDataPtr_;
    LOG4CXX_DEBUG(logger_, "Event timestamp base address [" << std::hex << eventIDPtr_ << "]");
	// Point the ID ptr to the end of the timestamps
	eventIDPtr_ = (uint32_t *)rawDataPtr_;
	eventIDPtr_ += numberOfPoints_ * (sizeof(uint64_t) / sizeof(uint32_t));
    LOG4CXX_DEBUG(logger_, "Event ID base address [" << std::hex << eventIDPtr_ << "]");
	// Point the energy pointer to the end of the IDs
	eventEnergyPtr_ = (uint32_t *)rawDataPtr_;
	eventEnergyPtr_ += numberOfPoints_ * (sizeof(uint64_t) / sizeof(uint32_t) + 1);
    LOG4CXX_DEBUG(logger_, "Event energy base address [" << std::hex << eventEnergyPtr_ << "]");
}

LATRDBuffer::~LATRDBuffer()
{
	// Free the allocated memory block
	if (rawDataPtr_){
		free(rawDataPtr_);
	}
}

boost::shared_ptr<Frame> LATRDBuffer::appendData(uint64_t *ts_ptr, uint32_t *id_ptr, uint32_t *energy_ptr, size_t qty_pts)
{
	boost::shared_ptr<Frame> frame;
	uint64_t *int_ts_ptr = eventTimestampPtr_;
	uint32_t *int_id_ptr = eventIDPtr_;
	uint32_t *int_energy_ptr = eventEnergyPtr_;
	// Add points to the current memory block.  If the block is filled then create a new
	// Frame object and reset the memory block.

    LOG4CXX_DEBUG(logger_, "Quantity of points to append [" << qty_pts << "]");
    LOG4CXX_DEBUG(logger_, "Timestamp base address [" << std::hex << int_ts_ptr << "]");
    int_ts_ptr += currentPoint_;
    LOG4CXX_DEBUG(logger_, "Timestamp current address [" << std::hex << int_ts_ptr << "]");
    LOG4CXX_DEBUG(logger_, "ID base address [" << std::hex << int_id_ptr << "]");
    int_id_ptr += currentPoint_;
    LOG4CXX_DEBUG(logger_, "ID current address [" << std::hex << int_id_ptr << "]");
    LOG4CXX_DEBUG(logger_, "Energy base address [" << std::hex << int_energy_ptr << "]");
    int_energy_ptr += currentPoint_;
    LOG4CXX_DEBUG(logger_, "Energy current address [" << std::hex << int_energy_ptr << "]");
	if (qty_pts < (numberOfPoints_ - currentPoint_)){
		// The whole block can be added without filling a buffer
		memcpy(int_ts_ptr, ts_ptr, qty_pts*sizeof(uint64_t));
		memcpy(int_id_ptr, id_ptr, qty_pts*sizeof(uint32_t));
		memcpy(int_energy_ptr, energy_ptr, qty_pts*sizeof(uint32_t));
		currentPoint_ += qty_pts;
	} else {
		// Fill the rest of the buffer, create a frame and then fill from the start
		size_t qty_to_fill = qty_pts - (numberOfPoints_ - currentPoint_);
		memcpy(int_ts_ptr, ts_ptr, qty_to_fill*sizeof(uint64_t));
		memcpy(int_id_ptr, id_ptr, qty_to_fill*sizeof(uint32_t));
		memcpy(int_energy_ptr, energy_ptr, qty_to_fill*sizeof(uint32_t));

		// The buffer should now be full so create the frame and copy the buffer in
		frame = boost::shared_ptr<Frame>(new Frame(frameName_));
		frame->copy_data(rawDataPtr_, numberOfPoints_ * bytes_per_data_point);

		// Reset the buffer
		memset(rawDataPtr_, 0, numberOfPoints_ * bytes_per_data_point);
		currentPoint_ = 0;

		// Calculate the remaining points left to fill
		qty_pts -= qty_to_fill;

		// Fill from the beginning of the buffer with the remaining points
		memcpy(eventTimestampPtr_, ts_ptr, qty_pts*sizeof(uint64_t));
		memcpy(eventIDPtr_, id_ptr, qty_pts*sizeof(uint32_t));
		memcpy(eventEnergyPtr_, energy_ptr, qty_pts*sizeof(uint32_t));
		currentPoint_ += qty_pts;
	}
	return frame;
}

} /* namespace FrameProcessor */
