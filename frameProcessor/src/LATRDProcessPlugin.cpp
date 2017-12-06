/*
 * LATRDProcessPlugin.cpp
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#include "LATRDProcessPlugin.h"

namespace FrameProcessor
{
const std::string LATRDProcessPlugin::META_NAME                  = "TristanProcessor";

const std::string LATRDProcessPlugin::CONFIG_PROCESS             = "process";
const std::string LATRDProcessPlugin::CONFIG_PROCESS_NUMBER      = "number";
const std::string LATRDProcessPlugin::CONFIG_PROCESS_RANK        = "rank";

LATRDProcessPlugin::LATRDProcessPlugin() :
		concurrent_processes_(1),
		concurrent_rank_(0),
		current_point_index_(0),
		current_time_slice_(0)
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FW.LATRDProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "LATRDProcessPlugin constructor.");

    // Create the work queue for processing jobs
    jobQueue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > >(new WorkQueue<boost::shared_ptr<LATRDProcessJob> >);
    // Create the work queue for completed jobs
    resultsQueue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > >(new WorkQueue<boost::shared_ptr<LATRDProcessJob> >);

    // Create a stack of process job objects ready to work
    for (int index = 0; index < LATRD::num_primary_packets*2; index++){
    	jobStack_.push(boost::shared_ptr<LATRDProcessJob>(new LATRDProcessJob(LATRD::primary_packet_size/sizeof(uint64_t))));
    }

    // Create the buffer managers
    timeStampBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_time_offset", UINT64_TYPE));
    idBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_id", UINT32_TYPE));
    energyBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_energy", UINT32_TYPE));

    // Create the meta header document
    this->createMetaHeader();

    // Configure threads for processing
    // Now start the worker thread to monitor the queue
    for (size_t index = 0; index < LATRD::number_of_processing_threads; index++){
    	thread_[index] = new boost::thread(&LATRDProcessPlugin::processTask, this);
    }
}

LATRDProcessPlugin::~LATRDProcessPlugin()
{
	// TODO Auto-generated destructor stub
}

/**
 * Set configuration options for the LATRD processing plugin.
 *
 * This sets up the process plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_PROCESS - Calls the method processConfig
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void LATRDProcessPlugin::configure(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Protect this method
  boost::lock_guard<boost::recursive_mutex> lock(mutex_);

  LOG4CXX_DEBUG(logger_, config.encode());

  // Check to see if we are configuring the process number and rank
  if (config.has_param(LATRDProcessPlugin::CONFIG_PROCESS)) {
    OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(LATRDProcessPlugin::CONFIG_PROCESS));
    this->configureProcess(processConfig, reply);
  }
}

/**
 * Set configuration options for the LATRD process count.
 *
 * This sets up the process plugin according to the configuration IpcMessage
 * objects that are received. The options are searched for:
 * CONFIG_PROCESS_NUMBER - Sets the number of writer processes executing
 * CONFIG_PROCESS_RANK - Sets the rank of this process
 *
 * The configuration is not applied if the writer is currently writing.
 *
 * \param[in] config - IpcMessage containing configuration data.
 * \param[out] reply - Response IpcMessage.
 */
void LATRDProcessPlugin::configureProcess(OdinData::IpcMessage& config, OdinData::IpcMessage& reply)
{
  // Check for process number and rank number
  if (config.has_param(LATRDProcessPlugin::CONFIG_PROCESS_NUMBER)) {
    this->concurrent_processes_ = config.get_param<size_t>(LATRDProcessPlugin::CONFIG_PROCESS_NUMBER);
    LOG4CXX_DEBUG(logger_, "Concurrent processes changed to " << this->concurrent_processes_);
  }
  if (config.has_param(LATRDProcessPlugin::CONFIG_PROCESS_RANK)) {
    this->concurrent_rank_ = config.get_param<size_t>(LATRDProcessPlugin::CONFIG_PROCESS_RANK);
    LOG4CXX_DEBUG(logger_, "Process rank changed to " << this->concurrent_rank_);
  }
  this->timeStampBuffer_->configureProcess(this->concurrent_processes_, this->concurrent_rank_);
  this->idBuffer_->configureProcess(this->concurrent_processes_, this->concurrent_rank_);
  this->energyBuffer_->configureProcess(this->concurrent_processes_, this->concurrent_rank_);

  this->createMetaHeader();
}

void LATRDProcessPlugin::createMetaHeader()
{
    // Create status message header
    meta_document_.SetObject();

    // Add rank and number of processes
    rapidjson::Value keyRank("process_rank", meta_document_.GetAllocator());
    rapidjson::Value valueRank;
    valueRank.SetInt(this->concurrent_rank_);
    meta_document_.AddMember(keyRank, valueRank, meta_document_.GetAllocator());
    rapidjson::Value keyNumber("process_number", meta_document_.GetAllocator());
    rapidjson::Value valueNumber;
    valueNumber.SetInt(this->concurrent_processes_);
    meta_document_.AddMember(keyNumber, valueNumber, meta_document_.GetAllocator());

}

boost::shared_ptr<LATRDProcessJob> LATRDProcessPlugin::getJob()
{
	boost::shared_ptr<LATRDProcessJob> job;
	// Check if we have a job available
	if (jobStack_.size() > 0){
		job = jobStack_.top();
		jobStack_.pop();
	} else{
		// No job available so create a new one
		job = boost::shared_ptr<LATRDProcessJob>(new LATRDProcessJob(LATRD::primary_packet_size/sizeof(uint64_t)));
	}
	return job;
}

void LATRDProcessPlugin::releaseJob(boost::shared_ptr<LATRDProcessJob> job)
{
	// Reset the job
	job->reset();
	// Place the job back on the stack ready for re-use
	jobStack_.push(job);
}

void LATRDProcessPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
	LATRD::PacketHeader packet_header;
	boost::shared_ptr<Frame> processedFrame;
	LOG4CXX_TRACE(logger_, "Processing raw frame.");

	// Extract the header from the buffer and print the details
	const LATRD::FrameHeader* hdrPtr = static_cast<const LATRD::FrameHeader*>(frame->get_data());
	LOG4CXX_TRACE(logger_, "Raw Frame Number: " << hdrPtr->frame_number);
	LOG4CXX_TRACE(logger_, "Frame State: " << hdrPtr->frame_state);
	LOG4CXX_TRACE(logger_, "Packets Received: " << hdrPtr->packets_received);

	// Extract the header words from each packet
	uint8_t *payload_ptr = (uint8_t*)(frame->get_data()) + sizeof(LATRD::FrameHeader);

	// Number of packet header 64bit words
	uint16_t packet_header_count = LATRD::packet_header_size/sizeof(uint64_t);

	for (int index = 0; index < LATRD::num_primary_packets; index++){
		if (hdrPtr->packet_state[index] == 0){
			LOG4CXX_DEBUG(logger_, "   Packet number: [Missing Packet]");
		} else {
			packet_header.headerWord1 = *(uint64_t *)payload_ptr;
			packet_header.headerWord2 = *(((uint64_t *)payload_ptr)+1);
			LOG4CXX_DEBUG(logger_, "   Header Word 1: 0x" << std::hex << packet_header.headerWord1);
			LOG4CXX_DEBUG(logger_, "   Header Word 2: 0x" << std::hex << packet_header.headerWord2);

			// We need to decode how many values are in the packet
			uint32_t packet_number = get_packet_number(packet_header.headerWord2);
			uint16_t word_count = get_word_count(packet_header.headerWord1);
			uint32_t time_slice = get_time_slice(packet_header.headerWord1);

			LOG4CXX_DEBUG(logger_, "   Packet number: " << packet_number);
			LOG4CXX_DEBUG(logger_, "   Word count: " << word_count);
			LOG4CXX_DEBUG(logger_, "   Time Slice ID: " << time_slice);

			uint16_t words_to_process = word_count - packet_header_count;
			// Loop over words to process
			uint64_t *data_ptr = (uint64_t *)payload_ptr;
			data_ptr += packet_header_count;
			boost::shared_ptr<LATRDProcessJob> job = this->getJob();
			job->job_id = index;
			job->data_ptr = data_ptr;
			job->time_slice = time_slice;
			job->words_to_process = words_to_process;
			jobQueue_->add(job);
		}
		payload_ptr += LATRD::primary_packet_size;
	}
	// Now we need to reconstruct the full dataset from the individual processing jobs
	std::map<uint32_t, boost::shared_ptr<LATRDProcessJob> > results;
	uint32_t qty_data_points = 0;
	while (results.size() < LATRD::num_primary_packets){
		boost::shared_ptr<LATRDProcessJob> job = resultsQueue_->remove();
		qty_data_points += job->valid_results;
	    LOG4CXX_DEBUG(logger_, "Processing job [" << job->job_id << "] completed with " << job->timestamp_mismatches << " timestamp mismatches");
		results[job->job_id] = job;
	}
    LOG4CXX_DEBUG(logger_, "Total number of valid data points [" << qty_data_points << "]");

    // Loop over the jobs in order, appending the results to the buffer.  If a buffer is filled
    // then a frame is created and can be pushed onto the next plugin.
	for (uint32_t index = 0; index < LATRD::num_primary_packets; index++){
		boost::shared_ptr<LATRDProcessJob> job = results[index];
		// Check if the time slice has changed
		if (job->time_slice != current_time_slice_){
			// Record the time slice index and time
			LOG4CXX_DEBUG(logger_, "*** New Time Slice ID: " << job->time_slice << " at index " << current_point_index_ << " at time " << job->event_ts_ptr[0]);
			// The new time slice needs to be published as meta data
			rapidjson::StringBuffer buffer;
			rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
			meta_document_.Accept(writer);
			publish_meta(META_NAME, "time_slice_ts", job->event_ts_ptr[0], buffer.GetString());
			publish_meta(META_NAME, "time_slice_index", current_point_index_, buffer.GetString());

			current_time_slice_ = job->time_slice;
		}
		// Copy the number of valid results into the buffer
		LOG4CXX_TRACE(logger_, "Appending "<< job->valid_results << " data points from job [" << job->job_id << "]");
		processedFrame = timeStampBuffer_->appendData(job->event_ts_ptr, job->valid_results);
		// Check if a full frame was returned by the append.  If it was then push it on to the next plugin.
		if (processedFrame){
			LOG4CXX_TRACE(logger_, "Pushing timestamp data frame.");
			std::vector<dimsize_t> dims(0);
			processedFrame->set_dataset_name("event_time_offset");
			processedFrame->set_data_type(3);
			processedFrame->set_dimensions("event_time_offset", dims);
			this->push(processedFrame);
		}

		processedFrame = idBuffer_->appendData(job->event_id_ptr, job->valid_results);
		// Check if a full frame was returned by the append.  If it was then push it on to the next plugin.
		if (processedFrame){
			LOG4CXX_TRACE(logger_, "Pushing ID data frame.");
			std::vector<dimsize_t> dims(0);
			processedFrame->set_dataset_name("event_id");
			processedFrame->set_data_type(2);
			processedFrame->set_dimensions("event_id", dims);
			this->push(processedFrame);
		}

		processedFrame = energyBuffer_->appendData(job->event_energy_ptr, job->valid_results);
		// Check if a full frame was returned by the append.  If it was then push it on to the next plugin.
		if (processedFrame){
			LOG4CXX_TRACE(logger_, "Pushing energy data frame.");
			std::vector<dimsize_t> dims(0);
			processedFrame->set_dataset_name("event_energy");
			processedFrame->set_data_type(2);
			processedFrame->set_dimensions("event_energy", dims);
			this->push(processedFrame);
		}

		// Finally publish any control word meta data
		this->publishControlMetaData(job);

		// Increment the current point index by the number of valid results
		current_point_index_ += job->valid_results;

		// Processing job is complete, release it
		this->releaseJob(job);
	}
}

void LATRDProcessPlugin::processTask()
{
	LOG4CXX_TRACE(logger_, "Starting processing task with ID [" << boost::this_thread::get_id() << "]");
	bool executing = true;
	while (executing){
		boost::shared_ptr<LATRDProcessJob> job = jobQueue_->remove();
	    job->valid_results = 0;
	    job->valid_control_words = 0;
	    job->timestamp_mismatches = 0;
	    uint64_t previous_course_timestamp = 0;
	    uint64_t current_course_timestamp = 0;
	    LOG4CXX_DEBUG(logger_, "Processing job [" << job->job_id
	    		<< "] on task [" << boost::this_thread::get_id()
	    		<< "] : Data pointer [" << job->data_ptr << "]");
		uint64_t *data_word_ptr = job->data_ptr;
		uint64_t *event_ts_ptr = job->event_ts_ptr;
		uint32_t *event_id_ptr = job->event_id_ptr;
		uint32_t *event_energy_ptr = job->event_energy_ptr;
		uint64_t *ctrl_word_ptr = job->ctrl_word_ptr;
		uint32_t *ctrl_index_ptr = job->ctrl_index_ptr;
		// Verify the first word is an extended timestamp word
		if (getControlType(*data_word_ptr) != ExtendedTimestamp){
		    LOG4CXX_DEBUG(logger_, "*** ERROR in job [" << job->job_id << "].  The first word is not an extended timestamp");
		}
		for (uint16_t index = 0; index < job->words_to_process; index++){
			try
			{
				if (processDataWord(*data_word_ptr,
						&previous_course_timestamp,
						&current_course_timestamp,
						event_ts_ptr,
						event_id_ptr,
						event_energy_ptr)){
					// Increment the event ptrs and the valid result count
					event_ts_ptr++;
					event_id_ptr++;
					event_energy_ptr++;
					job->valid_results++;
				} else if (isControlWord(*data_word_ptr)){
					// Set the control word value
					*ctrl_word_ptr = *data_word_ptr;
					// Set the index value for the control word
					*ctrl_index_ptr = job->valid_results;
					//
				    LOG4CXX_DEBUG(logger_, "Control word processed: [" << *ctrl_word_ptr
				    		<< "] index [" << *ctrl_index_ptr << "]");
					// Increment the control pointers and the valid result count
					ctrl_word_ptr++;
					ctrl_index_ptr++;
					job->valid_control_words++;
				    LOG4CXX_DEBUG(logger_, "Valid control words [" << job->valid_control_words << "]");
				}
			}
			catch (LATRDTimestampMismatchException& ex)
			{
				job->timestamp_mismatches++;
			}
			data_word_ptr++;
		}
	    LOG4CXX_DEBUG(logger_, "Processing complete for job [" << job->job_id
	    		<< "] on task [" << boost::this_thread::get_id()
	    		<< "] : Number of valid results [" << job->valid_results << "]");
	    resultsQueue_->add(job);
	}
}

void LATRDProcessPlugin::publishControlMetaData(boost::shared_ptr<LATRDProcessJob> job)
{
    LOG4CXX_DEBUG(logger_, "**** Number of control words [" << job->valid_control_words << "]");
	if (job->valid_control_words > 0){
		// Prepare the meta data header information
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		meta_document_.Accept(writer);

		// Loop over the number of control words
		for (uint16_t index = 0; index < job->valid_control_words; index++){
			// For each control word publish the appropriate meta data
			publish_meta(META_NAME, "control_word", job->ctrl_word_ptr[index], buffer.GetString());
			publish_meta(META_NAME, "control_word_index", job->ctrl_index_ptr[index]+current_point_index_, buffer.GetString());
		}
	}
}

bool LATRDProcessPlugin::processDataWord(uint64_t data_word,
		uint64_t *previous_course_timestamp,
		uint64_t *current_course_timestamp,
		uint64_t *event_ts,
		uint32_t *event_id,
		uint32_t *event_energy)
{
	bool event_word = false;
	//LOG4CXX_DEBUG(logger_, "Data Word: 0x" << std::hex << data_word);
	if (isControlWord(data_word)){
		if (getControlType(data_word) == ExtendedTimestamp){
			LOG4CXX_DEBUG(logger_, "Parsing extended timestamp control word [" << std::hex << data_word << "]");
			*previous_course_timestamp = *current_course_timestamp;
			*current_course_timestamp = getCourseTimestamp(data_word);
			LOG4CXX_DEBUG(logger_, "New extended timestamp [" << std::dec << *current_course_timestamp << "]");
		}
	} else {
		*event_ts = getFullTimestmap(data_word, *previous_course_timestamp, *current_course_timestamp);
		*event_energy = getEnergy(data_word);
		*event_id = getPositionID(data_word);
		event_word = true;
	}
	return event_word;
}

bool LATRDProcessPlugin::isControlWord(uint64_t data_word)
{
	bool ctrlWord = false;
	if ((data_word & LATRD::control_word_mask) > 0){
		//LOG4CXX_DEBUG(logger_, "Control Word TRUE");
		ctrlWord = true;
	}
	return ctrlWord;
}

LATRDDataControlType LATRDProcessPlugin::getControlType(uint64_t data_word)
{
	LATRDDataControlType ctrl_type = Unknown;
	if (!isControlWord(data_word)){
		throw LATRDProcessingException("Data word is not a control word");
	}
    uint8_t ctrl_word = (data_word >> 58) & LATRD::control_type_mask;
    if (ctrl_word == LATRD::control_course_timestamp_mask){
    	ctrl_type = ExtendedTimestamp;
    } else if (ctrl_word == LATRD::control_header_0_mask){
    	ctrl_type = HeaderWord0;
    } else if (ctrl_word == LATRD::control_header_1_mask){
    	ctrl_type = HeaderWord1;
    }
    return ctrl_type;
}

uint64_t LATRDProcessPlugin::getCourseTimestamp(uint64_t data_word)
{
	if (!isControlWord(data_word)){
		throw LATRDProcessingException("Data word is not a control word");
	}
	return data_word & LATRD::course_timestamp_mask;
}

uint64_t LATRDProcessPlugin::getFineTimestamp(uint64_t data_word)
{
	if (isControlWord(data_word)){
		throw LATRDProcessingException("Data word is a control word, not an event");
	}
    return (data_word >> 14) & LATRD::fine_timestamp_mask;
}

uint16_t LATRDProcessPlugin::getEnergy(uint64_t data_word)
{
	if (isControlWord(data_word)){
		throw LATRDProcessingException("Data word is a control word, not an event");
	}
    return data_word & LATRD::energy_mask;
}

uint32_t LATRDProcessPlugin::getPositionID(uint64_t data_word)
{
	if (isControlWord(data_word)){
		throw LATRDProcessingException("Data word is a control word, not an event");
	}
    return (data_word >> 39) & LATRD::energy_mask;
}

uint64_t LATRDProcessPlugin::getFullTimestmap(uint64_t data_word, uint64_t prev_course, uint64_t course)
{
    uint64_t full_ts = 0;
    uint64_t fine_ts = 0;
    if (isControlWord(data_word)){
		throw LATRDProcessingException("Data word is a control word, not an event");
    }
    fine_ts = getFineTimestamp(data_word);
    if ((findTimestampMatch(course) == findTimestampMatch(fine_ts)) || (findTimestampMatch(course)+1 == findTimestampMatch(fine_ts))){
        full_ts = (course & LATRD::timestamp_match_mask) + fine_ts;
    	//LOG4CXX_DEBUG(logger_, "Full timestamp generated 1");
    } else if ((findTimestampMatch(prev_course) == findTimestampMatch(fine_ts)) || (findTimestampMatch(prev_course)+1 == findTimestampMatch(fine_ts))){
        full_ts = (prev_course & LATRD::timestamp_match_mask) + fine_ts;
    	//LOG4CXX_DEBUG(logger_, "Full timestamp generated 2");
    } else {
    	throw LATRDTimestampMismatchException("Timestamp mismatch");
    	//LOG4CXX_DEBUG(logger_, "Timestamp mismatch");
    }
    return full_ts;
}

uint8_t LATRDProcessPlugin::findTimestampMatch(uint64_t time_stamp)
{
    // This method will return the 2 bits required for
    // matching a course timestamp to an extended timestamp
    return (time_stamp >> 22) & 0x03;
}

uint32_t LATRDProcessPlugin::get_packet_number(uint64_t headerWord2) const
{
    // Extract relevant bits to obtain the packet count
	return headerWord2 & 0xFFFFFFFF;
}

uint16_t LATRDProcessPlugin::get_word_count(uint64_t headerWord1) const
{
    // Extract relevant bits to obtain the word count
    return headerWord1 & 0x7FF;
}

uint32_t LATRDProcessPlugin::get_time_slice(uint64_t headerWord1) const
{
    // Extract relevant bits to obtain the time slice ID
    return (headerWord1 & 0x3FFFFFFFC0000) >> 18;
}


} /* namespace filewriter */
