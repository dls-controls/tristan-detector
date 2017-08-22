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
#include "WorkQueue.h"
#include "LATRDDefinitions.h"
#include "LATRDExceptions.h"
#include "LATRDBuffer.h"
#include "ClassLoader.h"

namespace FrameProcessor
{
enum LATRDDataControlType {Unknown, HeaderWord0, HeaderWord1, ExtendedTimestamp};

struct process_job_t
{
	uint32_t job_id;
	uint16_t words_to_process;
	uint16_t valid_results;
	uint16_t timestamp_mismatches;
	uint64_t *data_ptr;
	uint64_t *event_ts_ptr;
	uint32_t *event_id_ptr;
	uint32_t *event_energy_ptr;
};

class LATRDProcessPlugin : public FrameProcessorPlugin
{
public:
	LATRDProcessPlugin();
	virtual ~LATRDProcessPlugin();
	void configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);
	void configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply);

private:

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
	boost::shared_ptr<WorkQueue<process_job_t> > jobQueue_;
	boost::shared_ptr<WorkQueue<process_job_t> > resultsQueue_;

	/** Pointer to LATRD buffer and frame manager */
	boost::shared_ptr<LATRDBuffer> timeStampBuffer_;
	boost::shared_ptr<LATRDBuffer> idBuffer_;
	boost::shared_ptr<LATRDBuffer> energyBuffer_;

	size_t concurrent_processes_;
	size_t concurrent_rank_;

	void processFrame(boost::shared_ptr<Frame> frame);
	void processTask();
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
	uint32_t get_packet_number(void *headerWord2) const;
	uint16_t get_word_count(void *headerWord1) const;
};

} /* namespace filewriter */

#endif /* FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_ */
