/*
 * LATRDBuffer.h
 *
 *  Created on: 15 Aug 2017
 *      Author: gnx91527
 *
 *  The LATRD Buffer class is responsible for keeping track of data points
 *  and producing full frames whenever one is available.
 *  Internally a block of memory is allocated that can store a full frame.
 */

#ifndef FRAMEPROCESSOR_SRC_LATRDBUFFER_H_
#define FRAMEPROCESSOR_SRC_LATRDBUFFER_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
#include <string>
#include "Frame.h"
#include "LATRDExceptions.h"

using namespace log4cxx;
using namespace log4cxx::helpers;

namespace FrameProcessor
{
enum LATRDBufferType {UINT64_TYPE, UINT32_TYPE, UINT16_TYPE};

class LATRDBuffer {

public:
	LATRDBuffer(size_t numberOfDataPoints, const std::string& frame, LATRDBufferType type);
	virtual ~LATRDBuffer();
	boost::shared_ptr<Frame> appendData(void *data_ptr, size_t qty_pts);
	boost::shared_ptr<Frame> retrieveCurrentFrame();
	void configureProcess(size_t processes, size_t rank);
    void resetFrameNumber();

private:
	uint8_t* rawDataPtr_;
	size_t numberOfPoints_;
	size_t currentPoint_;
	std::string frameName_;
	LATRDBufferType type_;
	size_t dataSize_;
	uint64_t frameNumber_;
	size_t concurrent_processes_;
	size_t concurrent_rank_;

	/** Pointer to logger */
	LoggerPtr logger_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_LATRDBUFFER_H_ */
