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

#include <stack>
#include "FrameProcessorPlugin.h"
#include "WorkQueue.h"
#include "LATRDDefinitions.h"
#include "LATRDExceptions.h"
#include "LATRDBuffer.h"
#include "LATRDProcessJob.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
enum LATRDDataControlType {Unknown, HeaderWord0, HeaderWord1, ExtendedTimestamp};

class LATRDProcessPlugin : public FrameProcessorPlugin
{
public:
	LATRDProcessPlugin();
	virtual ~LATRDProcessPlugin();
	void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
	void configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
	void createMetaHeader();

private:

	/** Constant for this decoders name when publishing meta data */
	static const std::string META_NAME;

  /** Configuration constant for setting raw mode */
  static const std::string CONFIG_RAW_MODE;

  /** Configuration constant for process related items */
	static const std::string CONFIG_PROCESS;
	/** Configuration constant for number of processes */
	static const std::string CONFIG_PROCESS_NUMBER;
	/** Configuration constant for this process rank */
	static const std::string CONFIG_PROCESS_RANK;

	/** Pointer to logger */
	LoggerPtr logger_;
	/** Mutex used to make this class thread safe */
	boost::recursive_mutex mutex_;

	/** Pointer to worker queue thread */
	boost::thread *thread_[LATRD::number_of_processing_threads];

	/** Pointers to job queues for processing packets and results notification */
	boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > jobQueue_;
	boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > resultsQueue_;

	/** Stack of processing job objects **/
	std::stack<boost::shared_ptr<LATRDProcessJob> > jobStack_;

	/** Pointer to LATRD buffer and frame manager */
  boost::shared_ptr<LATRDBuffer> rawBuffer_;
  boost::shared_ptr<LATRDBuffer> timeStampBuffer_;
	boost::shared_ptr<LATRDBuffer> idBuffer_;
	boost::shared_ptr<LATRDBuffer> energyBuffer_;

    rapidjson::Document meta_document_;

	size_t concurrent_processes_;
	size_t concurrent_rank_;

	/** Management of time slice information **/
	uint64_t current_point_index_;
	uint32_t current_time_slice_;

	/** Process raw mode **/
	uint32_t raw_mode_;

	boost::shared_ptr<LATRDProcessJob> getJob();
	void releaseJob(boost::shared_ptr<LATRDProcessJob> job);

  void process_frame(boost::shared_ptr<Frame> frame);
  void process_raw(boost::shared_ptr<Frame> frame);
	void processTask();
	void publishControlMetaData(boost::shared_ptr<LATRDProcessJob> job);
	bool processDataWord(uint64_t data_word,
			uint64_t *previous_course_timestamp,
			uint64_t *current_course_timestamp,
			uint64_t *event_ts,
			uint32_t *event_id,
			uint32_t *event_energy);
	bool isControlWord(uint64_t data_word);
	LATRDDataControlType getControlType(uint64_t data_word);
	uint64_t getCourseTimestamp(uint64_t data_word);
	uint64_t getFineTimestamp(uint64_t data_word);
	uint16_t getEnergy(uint64_t data_word);
	uint32_t getPositionID(uint64_t data_word);
	uint64_t getFullTimestmap(uint64_t data_word, uint64_t prev_course, uint64_t course);
	uint8_t findTimestampMatch(uint64_t time_stamp);
	uint32_t get_packet_number(uint64_t headerWord2) const;
	uint16_t get_word_count(uint64_t headerWord1) const;
	uint32_t get_time_slice(uint64_t headerWord1) const;
};

} /* namespace filewriter */

#endif /* FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_ */
