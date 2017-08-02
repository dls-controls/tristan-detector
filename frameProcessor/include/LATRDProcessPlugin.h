/*
 * LATRDProcessPlugin.h
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#ifndef FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_
#define FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>
using namespace log4cxx;
using namespace log4cxx::helpers;

#include "FrameProcessorPlugin.h"
#include "LATRDDefinitions.h"
#include "ClassLoader.h"

namespace FrameProcessor
{

class LATRDProcessPlugin : public FrameProcessorPlugin
{
public:
	LATRDProcessPlugin();
	virtual ~LATRDProcessPlugin();

private:
  void processFrame(boost::shared_ptr<Frame> frame);
  void processDataWord(uint64_t data_word);
  uint32_t get_packet_number(void *headerWord2) const;
  uint16_t get_word_count(void *headerWord1) const;

  /** Pointer to logger */
  LoggerPtr logger_;
};

} /* namespace filewriter */

#endif /* FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_ */
