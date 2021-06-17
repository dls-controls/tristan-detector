/*
 * LATRDBuffer.cpp
 *
 *  Created on: 15 Aug 2017
 *      Author: gnx91527
 */

#include "LATRDBuffer.h"
#include "FrameMetaData.h"
#include "DataBlockFrame.h"
#include "DebugLevelLogger.h"

namespace FrameProcessor {

LATRDBuffer::LATRDBuffer(size_t numberOfDataPoints, const std::string& frame, LATRDBufferType type) :
		numberOfPoints_(numberOfDataPoints),
		currentPoint_(0),
		frameName_(frame),
		type_(type),
		frameNumber_(0),
		concurrent_processes_(1),
		concurrent_rank_(0)
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.LATRDBuffer");
    LOG4CXX_TRACE(logger_, "LATRDBuffer constructor.");

    // Allocate the memory block required for the points
    LOG4CXX_DEBUG(logger_, "Number of data points [" << numberOfPoints_ << "]");
    size_t bytes_to_allocate = numberOfPoints_;
    switch (type_)
    {
    case UINT64_TYPE:
    	bytes_to_allocate = bytes_to_allocate * sizeof(uint64_t);
    	dataSize_ = sizeof(uint64_t);
    	break;
    case UINT32_TYPE:
    	bytes_to_allocate = bytes_to_allocate * sizeof(uint32_t);
    	dataSize_ = sizeof(uint32_t);
    	break;
	case UINT16_TYPE:
		bytes_to_allocate = bytes_to_allocate * sizeof(uint16_t);
		dataSize_ = sizeof(uint16_t);
		break;
    default:
    	throw LATRDProcessingException("Unknown datatype specified");
    }
    LOG4CXX_DEBUG(logger_, "Total bytes to allocate [" << bytes_to_allocate << "]");
	rawDataPtr_ = static_cast<uint8_t*>(malloc(bytes_to_allocate));
    LOG4CXX_DEBUG(logger_, "Allocated memory block with base address [" << std::hex << rawDataPtr_ << "]");
}

LATRDBuffer::~LATRDBuffer()
{
	// Free the allocated memory block
	if (rawDataPtr_){
		free(rawDataPtr_);
	}
}

boost::shared_ptr<Frame> LATRDBuffer::appendData(void *data_ptr, size_t qty_pts)
{
	boost::shared_ptr<Frame> frame;

	// Add points to the current memory block.  If the block is filled then create a new
	// Frame object and reset the memory block.

	char *char_data_ptr = (char *)rawDataPtr_;

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Quantity of points to append to " << this->frameName_ << " is [" << qty_pts << "]");
    char_data_ptr += (currentPoint_ * dataSize_);
	if (qty_pts < (numberOfPoints_ - currentPoint_)){
		// The whole block can be added without filling a buffer
		memcpy(char_data_ptr, data_ptr, qty_pts * dataSize_);
		currentPoint_ += qty_pts;
	} else {
		// Fill the rest of the buffer, create a frame and then fill from the start
		size_t qty_to_fill = numberOfPoints_ - currentPoint_;
	    LOG4CXX_DEBUG(logger_, "Current fill point " << currentPoint_ << ", filling buffer with " << qty_to_fill << " points");
		memcpy(char_data_ptr, data_ptr, qty_to_fill * dataSize_);
        currentPoint_ += qty_to_fill;

        frame = retrieveCurrentFrame();

		// Calculate the remaining points left to fill
		qty_pts -= qty_to_fill;
	    LOG4CXX_DEBUG(logger_, "Remaining points to fill " << qty_pts);
		char_data_ptr = (char *)data_ptr;
		char_data_ptr += (qty_to_fill * dataSize_);
		// Fill from the beginning of the buffer with the remaining points
		memcpy(rawDataPtr_, char_data_ptr, qty_pts * dataSize_);
		currentPoint_ += qty_pts;
	}
	return frame;
}

boost::shared_ptr<Frame> LATRDBuffer::retrieveCurrentFrame()
{
	boost::shared_ptr<Frame> frame;
	if (currentPoint_ == 0 || numberOfPoints_ < currentPoint_)
    {
		LOG4CXX_DEBUG_LEVEL(2, logger_, "No frame created from Idle buffer as there were no data points");
        return frame;
    }

    LOG4CXX_DEBUG(logger_, "Creating a new frame for [" << frameName_ << "] of qty " << currentPoint_);

    int items_to_zero = (numberOfPoints_ - currentPoint_);
	memset(rawDataPtr_ + (currentPoint_ * dataSize_), 0, items_to_zero * dataSize_);
    
	FrameProcessor::FrameMetaData frame_meta;

    // the support for single-dimension frames in odin could be better; we must set dims to empty here.
    std::vector<dimsize_t> dims;
    frame_meta.set_dataset_name(frameName_);
	switch (type_)
	{
	case UINT64_TYPE:
        frame_meta.set_data_type(FrameProcessor::raw_64bit);
		break;
	case UINT32_TYPE:
        frame_meta.set_data_type(FrameProcessor::raw_32bit);
		break;
	case UINT16_TYPE:
        frame_meta.set_data_type(FrameProcessor::raw_16bit);
		break;
	default:
		throw LATRDProcessingException("Unknown datatype specified");
	}
    frame_meta.set_dimensions(dims);
    frame_meta.set_compression_type(FrameProcessor::no_compression);

	frame = boost::shared_ptr<Frame>(new DataBlockFrame(frame_meta, rawDataPtr_, numberOfPoints_ * dataSize_));

	frame->set_frame_number(concurrent_rank_ + (frameNumber_ * concurrent_processes_));

    LOG4CXX_DEBUG_LEVEL(1, logger_, "frame number is:" << frame->get_frame_number());
	++frameNumber_;

	currentPoint_ = 0;

    return frame;
}

void LATRDBuffer::configureProcess(size_t processes, size_t rank)
{
	concurrent_processes_ = processes;
	concurrent_rank_ = rank;
}

void LATRDBuffer::resetFrameNumber()
{
  frameNumber_ = 0;
}

} /* namespace FrameProcessor */
