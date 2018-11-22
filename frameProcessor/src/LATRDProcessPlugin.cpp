/*
 * LATRDProcessPlugin.cpp
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#include <LATRDDefinitions.h>
#include "LATRDProcessPlugin.h"
#include "DebugLevelLogger.h"

namespace FrameProcessor
{
const std::string LATRDProcessPlugin::META_NAME                  = "TristanProcessor";

const std::string LATRDProcessPlugin::CONFIG_MODE                = "mode";
const std::string LATRDProcessPlugin::CONFIG_MODE_TIME_ENERGY    = "time_energy";
const std::string LATRDProcessPlugin::CONFIG_MODE_TIME_ONLY      = "time_only";
const std::string LATRDProcessPlugin::CONFIG_MODE_COUNT          = "count";

const std::string LATRDProcessPlugin::CONFIG_RAW_MODE            = "raw_mode";

const std::string LATRDProcessPlugin::CONFIG_RESET_FRAME         = "reset_frame";

const std::string LATRDProcessPlugin::CONFIG_PROCESS             = "process";
const std::string LATRDProcessPlugin::CONFIG_PROCESS_NUMBER      = "number";
const std::string LATRDProcessPlugin::CONFIG_PROCESS_RANK        = "rank";

const std::string LATRDProcessPlugin::CONFIG_SENSOR              = "sensor";
const std::string LATRDProcessPlugin::CONFIG_SENSOR_WIDTH        = "width";
const std::string LATRDProcessPlugin::CONFIG_SENSOR_HEIGHT       = "height";

  LATRDProcessPlugin::LATRDProcessPlugin() :
    sensor_width_(256),
    sensor_height_(256),
    mode_(CONFIG_MODE_TIME_ENERGY),
	concurrent_processes_(1),
	concurrent_rank_(0),
	current_point_index_(0),
	current_time_slice_(0),
//    last_processed_ts_wrap_(0),
//    last_processed_ts_buffer_(0),
//    last_processed_frame_number_(0),
//    last_processed_was_idle_(0),
    raw_mode_(0)
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.LATRDProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "LATRDProcessPlugin constructor.");

    integral_.init(sensor_width_, sensor_height_);
    integral_.reset_image();

    // Create the work queue for processing jobs
//    jobQueue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > >(new WorkQueue<boost::shared_ptr<LATRDProcessJob> >);
    // Create the work queue for completed jobs
//    resultsQueue_ = boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > >(new WorkQueue<boost::shared_ptr<LATRDProcessJob> >);

    // Create a stack of process job objects ready to work
//    for (int index = 0; index < LATRD::num_primary_packets*2; index++){
//    	jobStack_.push(boost::shared_ptr<LATRDProcessJob>(new LATRDProcessJob(LATRD::primary_packet_size/sizeof(uint64_t))));
//    }

    // Create the buffer managers
    rawBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "raw_data", UINT64_TYPE));
//    timeStampBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_time_offset", UINT64_TYPE));
//    idBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_id", UINT32_TYPE));
//    energyBuffer_ = boost::shared_ptr<LATRDBuffer>(new LATRDBuffer(LATRD::frame_size, "event_energy", UINT32_TYPE));

    // Create the meta header document
//    this->createMetaHeader();

  LOG4CXX_DEBUG_LEVEL(1, logger_, "Completed LATRDProcessPlugin constructor.");
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
  LOG4CXX_DEBUG_LEVEL(1, logger_, config.encode());

  // Check for operational mode
  if (config.has_param(LATRDProcessPlugin::CONFIG_MODE)) {
    std::string mode = config.get_param<std::string>(LATRDProcessPlugin::CONFIG_MODE);
    if (mode == LATRDProcessPlugin::CONFIG_MODE_TIME_ENERGY ||
        mode == LATRDProcessPlugin::CONFIG_MODE_TIME_ONLY ||
        mode == LATRDProcessPlugin::CONFIG_MODE_COUNT){
      this->mode_ = mode;
      LOG4CXX_DEBUG_LEVEL(1, logger_, "Operational mode set to " << this->mode_);
    } else {
      LOG4CXX_ERROR(logger_, "Invalid operational mode requested: " << mode);
    }
  }

  // Check for raw mode
  if (config.has_param(LATRDProcessPlugin::CONFIG_RAW_MODE)) {
    this->raw_mode_ = config.get_param<uint32_t>(LATRDProcessPlugin::CONFIG_RAW_MODE);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Raw mode set to " << this->raw_mode_);
  }

  // Check for a frame reset
  if (config.has_param(LATRDProcessPlugin::CONFIG_RESET_FRAME)) {
    rawBuffer_->resetFrameNumber();
  }

  // Check to see if we are configuring the process number and rank
  if (config.has_param(LATRDProcessPlugin::CONFIG_PROCESS)) {
    OdinData::IpcMessage processConfig(config.get_param<const rapidjson::Value&>(LATRDProcessPlugin::CONFIG_PROCESS));
    this->configureProcess(processConfig, reply);
  }

  // Check to see if we are configuring the sensor width and height
  if (config.has_param(LATRDProcessPlugin::CONFIG_SENSOR)) {
    OdinData::IpcMessage sensorConfig(config.get_param<const rapidjson::Value&>(LATRDProcessPlugin::CONFIG_SENSOR));
    this->configureSensor(sensorConfig, reply);
  }

}

void LATRDProcessPlugin::requestConfiguration(OdinData::IpcMessage& reply)
{
  // Return the configuration of the LATRD process plugin
  reply.set_param(get_name() + "/" + LATRDProcessPlugin::CONFIG_MODE, this->mode_);
  reply.set_param(get_name() + "/" + LATRDProcessPlugin::CONFIG_RAW_MODE, this->raw_mode_);
}

void LATRDProcessPlugin::status(OdinData::IpcMessage& status)
{
  uint32_t processed_jobs = 0;
  uint32_t job_q_size = 0;
  uint32_t result_q_size = 0;
  uint32_t processed_frames = 0;
  uint32_t output_frames = 0;
  // Return the status of the LATRD process plugin
  this->coordinator_.get_statistics(&processed_jobs, &job_q_size, &result_q_size, &processed_frames, &output_frames);
  status.set_param(get_name() + "/processed_jobs", processed_jobs);
  status.set_param(get_name() + "/job_queue", job_q_size);
  status.set_param(get_name() + "/results_queue", result_q_size);
  status.set_param(get_name() + "/processed_frames", processed_frames);
  status.set_param(get_name() + "/output_frames", output_frames);
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
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Concurrent processes changed to " << this->concurrent_processes_);
  }
  if (config.has_param(LATRDProcessPlugin::CONFIG_PROCESS_RANK)) {
    this->concurrent_rank_ = config.get_param<size_t>(LATRDProcessPlugin::CONFIG_PROCESS_RANK);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Process rank changed to " << this->concurrent_rank_);
  }
  this->coordinator_.configure_process(this->concurrent_processes_, this->concurrent_rank_);

  this->rawBuffer_->configureProcess(this->concurrent_processes_, this->concurrent_rank_);

  this->createMetaHeader();
}

void LATRDProcessPlugin::configureSensor(OdinData::IpcMessage &config, OdinData::IpcMessage &reply)
{
  // Check for sensor width and height
  if (config.has_param(LATRDProcessPlugin::CONFIG_SENSOR_WIDTH)) {
    this->sensor_width_ = config.get_param<size_t>(LATRDProcessPlugin::CONFIG_SENSOR_WIDTH);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Sensor width changed to " << this->sensor_width_);
  }

  if (config.has_param(LATRDProcessPlugin::CONFIG_SENSOR_HEIGHT)) {
    this->sensor_height_ = config.get_param<size_t>(LATRDProcessPlugin::CONFIG_SENSOR_HEIGHT);
    LOG4CXX_DEBUG_LEVEL(1, logger_, "Sensor height changed to " << this->sensor_height_);
  }

  integral_.init(this->sensor_width_, this->sensor_height_);
  integral_.reset_image();
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

/*boost::shared_ptr<LATRDProcessJob> LATRDProcessPlugin::getJob()
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
}*/

void LATRDProcessPlugin::dump_frame(boost::shared_ptr<Frame> frame)
{
    const LATRD::FrameHeader* hdrPtr = static_cast<const LATRD::FrameHeader*>(frame->get_data());

    // Print a full report of the incoming frame to log
    LOG4CXX_DEBUG_LEVEL(2, logger_, "\n==============================================\n"
                           << "***              Frame Report              ***\n"
                           << "*** Frame number: " << frame->get_frame_number() << "\n"
                           << "*** Internal number: " << hdrPtr->frame_number << "\n"
                           << "*** State: " << hdrPtr->frame_state << "\n"
                           << "*** Idle flag: " << hdrPtr->idle_frame << "\n"
                           << "*** Packets Received: " << hdrPtr->packets_received << "\n"
                           << "=============================================="
                           );
}

void LATRDProcessPlugin::process_frame(boost::shared_ptr<Frame> frame)
{
  if (this->raw_mode_ == 1) {
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Raw mode selected for fast recording of packets");
    this->process_raw(frame);
  } else {
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Process frame called with mode: " << this->mode_);
    if (this->mode_ == LATRDProcessPlugin::CONFIG_MODE_COUNT) {
      std::vector <boost::shared_ptr<Frame> > frames = integral_.process_frame(frame);
      std::vector <boost::shared_ptr<Frame> >::iterator iter;
      for (iter = frames.begin(); iter != frames.end(); ++iter) {
        this->push(*iter);
      }
    } else {
      //this->dump_frame(frame);
      std::vector <boost::shared_ptr<Frame> > frames = coordinator_.process_frame(frame);
      std::vector <boost::shared_ptr<Frame> >::iterator iter;
      for (iter = frames.begin(); iter != frames.end(); ++iter) {
        this->push(*iter);
      }
    }
  }
}

void LATRDProcessPlugin::process_raw(boost::shared_ptr<Frame> frame)
{
  // TODO: Fudge factor of 8 introduced as packets are not formed correctly
  LATRD::PacketHeader packet_header;
  boost::shared_ptr<Frame> processedFrame;

  // Extract the header from the buffer and print the details
  const LATRD::FrameHeader* hdrPtr = static_cast<const LATRD::FrameHeader*>(frame->get_data());
  LOG4CXX_TRACE(logger_, "Raw Frame Number: " << hdrPtr->frame_number);
  LOG4CXX_TRACE(logger_, "Frame State: " << hdrPtr->frame_state);
  LOG4CXX_TRACE(logger_, "Packets Received: " << hdrPtr->packets_received);
  if (hdrPtr->idle_frame == 1){
    // Get whatever is available in the buffer and then push it on to the next plugin.
    LOG4CXX_ERROR(logger_, "** Idle frame passed to plugin !! **");
    processedFrame = rawBuffer_->retrieveCurrentFrame();
    if (processedFrame) {
      LOG4CXX_TRACE(logger_, "Pushing a raw data frame due to an idle flag.");
      std::vector<dimsize_t> dims(0);
      processedFrame->set_dataset_name("raw_data");
      processedFrame->set_data_type(3);
      processedFrame->set_dimensions(dims);
      this->push(processedFrame);
    }
  } else {
    // Extract the header words from each packet
    uint8_t *payload_ptr = (uint8_t *) (frame->get_data()) + sizeof(LATRD::FrameHeader);

    // Number of packet header 64bit words
    uint16_t packet_header_count = (LATRD::packet_header_size / sizeof(uint64_t)) - 1;

    for (int index = 0; index < LATRD::num_primary_packets; index++) {
      if (hdrPtr->packet_state[index] == 0) {
//        LOG4CXX_DEBUG(logger_, "   Packet number: [Missing Packet]");
      } else {
        packet_header.headerWord1 = *(((uint64_t *) payload_ptr) + 1);
        packet_header.headerWord2 = *(((uint64_t *) payload_ptr) + 2);
        LOG4CXX_DEBUG_LEVEL(3, logger_, "   Header Word 1: 0x" << std::hex << packet_header.headerWord1);
        LOG4CXX_DEBUG_LEVEL(3, logger_, "   Header Word 2: 0x" << std::hex << packet_header.headerWord2);

        // We need to decode how many values are in the packet
        uint32_t packet_number = LATRD::get_packet_number(packet_header.headerWord2);
        uint16_t word_count = LATRD::get_word_count(packet_header.headerWord1) + (uint16_t)2;
        uint32_t time_slice = LATRD::get_time_slice_modulo(packet_header.headerWord1);

        // Copy the number of valid results into the buffer
        LOG4CXX_TRACE(logger_, "Appending " << word_count << " raw values");
        processedFrame = rawBuffer_->appendData(payload_ptr, word_count);
        // Check if a full frame was returned by the append.  If it was then push it on to the next plugin.
        if (processedFrame) {
          LOG4CXX_TRACE(logger_, "Pushing a raw data frame.");
          std::vector<dimsize_t> dims(0);
          processedFrame->set_dataset_name("raw_data");
          processedFrame->set_data_type(3);
          processedFrame->set_dimensions(dims);
          this->push(processedFrame);
        }
      }
      // Increment the payload_ptr by the correct number of bytes in a packet
      payload_ptr += LATRD::primary_packet_size;
    }
  }
}

void LATRDProcessPlugin::publishControlMetaData(boost::shared_ptr<LATRDProcessJob> job)
{
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Number of control words [" << job->valid_control_words << "]");
	if (job->valid_control_words > 0){
		// Prepare the meta data header information
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		meta_document_.Accept(writer);

		// Loop over the number of control words
		for (uint16_t index = 0; index < job->valid_control_words; index++){
			// For each control word publish the appropriate meta data
			//publish_meta(META_NAME, "control_word", job->ctrl_word_ptr[index], buffer.GetString());
			//publish_meta(META_NAME, "control_word_index", job->ctrl_index_ptr[index]+current_point_index_, buffer.GetString());
		}
	}
}

bool LATRDProcessPlugin::reset_statistics()
{
    coordinator_.reset_statistics();
    return true;
}

int LATRDProcessPlugin::get_version_major()
{
  // TOOD: This is a placeholder for when version information is added
  return 0;
}

int LATRDProcessPlugin::get_version_minor()
{
  // TOOD: This is a placeholder for when version information is added
  return 0;
}

int LATRDProcessPlugin::get_version_patch()
{
  // TOOD: This is a placeholder for when version information is added
  return 0;
}

std::string LATRDProcessPlugin::get_version_short()
{
  // TOOD: This is a placeholder for when version information is added
  return "0.0.0";
}

std::string LATRDProcessPlugin::get_version_long()
{
  // TOOD: This is a placeholder for when version information is added
  return "0.0.0";
}

} /* namespace FrameProcesser */
