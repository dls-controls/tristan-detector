/*
 * LATRDFileWriter.cpp
 *
 *  Created on: 8 Aug 2017
 *      Author: gnx91527
 */

#include "LATRDFileWriterPlugin.h"

namespace FrameProcessor
{

LATRDFileWriterPlugin::LATRDFileWriterPlugin()
{
    // Setup logging for the class
	this->logger_ = Logger::getLogger("LATRDFileWriterPlugin");
	this->logger_->setLevel(Level::getTrace());
	LOG4CXX_TRACE(logger_, "LATRDFileWriterPlugin constructor.");
}

LATRDFileWriterPlugin::~LATRDFileWriterPlugin()
{
	// TODO Auto-generated destructor stub
}

} /* namespace FrameProcessor */

