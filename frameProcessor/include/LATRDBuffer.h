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
using namespace log4cxx;
using namespace log4cxx::helpers;

namespace FrameProcessor
{

class LATRDBuffer {

public:
    static const size_t bytes_per_data_point = sizeof(uint64_t) + sizeof(uint32_t) + sizeof(uint32_t);

	LATRDBuffer(size_t numberOfDataPoints, const std::string& frame);
	virtual ~LATRDBuffer();
	boost::shared_ptr<Frame> appendData(uint64_t *ts_ptr, uint32_t *id_ptr, uint32_t *energy_ptr, size_t qty_pts);

private:
	void *rawDataPtr_;
	uint64_t *eventTimestampPtr_;
	uint32_t *eventIDPtr_;
	uint32_t *eventEnergyPtr_;
	size_t numberOfPoints_;
	size_t currentPoint_;
	std::string frameName_;

	/** Pointer to logger */
	LoggerPtr logger_;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_LATRDBUFFER_H_ */
