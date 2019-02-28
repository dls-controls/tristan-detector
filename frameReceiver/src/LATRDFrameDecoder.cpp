/*
 * LATRDFrameDecoder.cpp
 *
 *  Created on: 23 Feb 2017
 *      Author: gnx91527
 */

#include <LATRDDefinitions.h>
#include "LATRDFrameDecoder.h"
#include "version.h"

namespace FrameReceiver
{

LATRDFrameDecoder::LATRDFrameDecoder() :
                FrameDecoderUDP(),
                current_frame_(1),
                packet_counter_(0),
        		current_frame_seen_(-1),
        		current_frame_buffer_id_(-1),
        		current_frame_buffer_(0),
        		current_frame_header_(0),
        		dropping_frame_data_(false),
        		frame_timeout_ms_(1000),
        		frames_timedout_(0)
{
    current_raw_packet_header_.reset(new uint8_t[LATRD::packet_header_size]);
    dropped_frame_buffer_.reset(new uint8_t[LATRD::total_frame_size]);

    this->logger_ = Logger::getLogger("FR.LATRDDecoderPlugin");
    LOG4CXX_INFO(logger_, "LATRDFrameDecoder version " << this->get_version_long() << " loaded");
}

void LATRDFrameDecoder::init(LoggerPtr& logger, OdinData::IpcMessage& config_msg)
{
	FrameDecoder::init(logger, config_msg);

    if (enable_packet_logging_) {
        LOG4CXX_INFO(packet_logger_, "PktHdr: SourceAddress");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 SourcePort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     DestinationPort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      Control [63:62]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    Type [61:58]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    Producer ID [57:50]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    Time Slice [49:18]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          Spare [17:10]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    Word Count [10:0]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    |      Control [63:62]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    |      |    Type [61:58]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    |      |    |    Spare [57:32]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    |      |    |    |          Packet Number [31:0]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |                 |     |      |    |    |    |          |    |      |    |    |          |          |");
    }
}

LATRDFrameDecoder::~LATRDFrameDecoder()
{
}

const size_t LATRDFrameDecoder::get_frame_buffer_size() const
{
    return LATRD::total_frame_size;
}

const size_t LATRDFrameDecoder::get_frame_header_size() const
{
    return sizeof(LATRD::FrameHeader);
}

const size_t LATRDFrameDecoder::get_packet_header_size() const
{
    return LATRD::packet_header_size;
}

void* LATRDFrameDecoder::get_packet_header_buffer()
{
    return current_raw_packet_header_.get();
}

void LATRDFrameDecoder::log_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    current_packet_header_.headerWord1 = *(((uint64_t *)raw_packet_header())+1);
    current_packet_header_.headerWord2 = *(((uint64_t *)raw_packet_header())+2);

    // Read the decoded header information into local variables for use
    uint32_t packetNumber = LATRD::get_packet_number(current_packet_header_.headerWord2);
    uint8_t producerID = LATRD::get_producer_ID(current_packet_header_.headerWord1);
    uint32_t timeSliceModulo = LATRD::get_time_slice_modulo(current_packet_header_.headerWord1);
    uint8_t timeSliceNumber = LATRD::get_time_slice_number(current_packet_header_.headerWord2);
    uint16_t wordCount = LATRD::get_word_count(current_packet_header_.headerWord1);

    // Dump raw header if packet logging enabled
    if (enable_packet_logging_){
        std::stringstream ss;
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex << std::setfill('0');

        // Obtain a pointer to the first 64 bit header word
        uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord1;

        // Extract relevant bits into the next available size data type
        uint8_t ctrlValue = (uint8_t)((hdr_ptr[0] & 0xC0) >> 6);
        uint8_t typeValue = (uint8_t)((hdr_ptr[0] & 0x3C) >> 2);
        uint8_t spareValue = (uint8_t)(((hdr_ptr[5] & 0x03) << 6) + ((hdr_ptr[6] & 0xF0) >> 2));

        // Format the data values into the stream
        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
           << " 0x" << std::setw(2) << unsigned(producerID) << " 0x" << std::setw(8) << timeSliceModulo << " 0x"
           << std::setw(2) << unsigned(spareValue) << " 0x" << std::setw(4) << wordCount;

        // Obtain a pointer to the second 64 bit header word
        hdr_ptr = (uint8_t *)&current_packet_header_.headerWord2;

        ctrlValue = (uint8_t)((hdr_ptr[0] & 0xC0) >> 6);
        typeValue = (uint8_t)((hdr_ptr[0] & 0x3C) >> 2);
        spareValue = (uint8_t)(((hdr_ptr[0] & 0x03) << 24)
                               + (hdr_ptr[1] << 16)
                               + (hdr_ptr[2] << 8)
                               + (hdr_ptr[3]));

        // Format the data values into the stream
        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
           << " 0x" << std::setw(8) << unsigned(spareValue) << " 0x" << std::setw(8) << unsigned(packetNumber);

        ss << std::dec;
        LOG4CXX_INFO(packet_logger_, ss.str());
    }

    LOG4CXX_DEBUG_LEVEL(2, logger_, "Got packet header for packet number: " << packetNumber);
    LOG4CXX_DEBUG_LEVEL(2, logger_, "Time slice modulo [" << timeSliceModulo << "] number [" << (uint32_t)timeSliceNumber << "]");
    LOG4CXX_DEBUG_LEVEL(3, logger_, "" << std::hex << std::setfill('0')
                                       << "Header 0x" << std::setw(8)
                                       << *(((uint64_t *)raw_packet_header())+1));

}

void LATRDFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
  //TODO validate header size and content, handle incoming new packet buffer allocation etc

  //log_packet(bytes_received, port, from_addr);

  // If we receive an IDLE frame then set the frame number to 0
    if ((*(((uint64_t *)raw_packet_header())+1)&LATRD::packet_header_idle_mask) == LATRD::packet_header_idle_mask){
        LOG4CXX_DEBUG_LEVEL(2, logger_, "  IDLE packet detected");
        // Mark this buffer as a last frame buffer
        current_frame_ = 0;
    } else {
      // Only increment the packet counter if we have a non idle packet
      packet_counter_++;
    }

    if (current_frame_ != current_frame_seen_){
    current_frame_seen_ = current_frame_;
    if (frame_buffer_map_.count(current_frame_seen_) == 0){
      if (empty_buffer_queue_.empty()){
        current_frame_buffer_ = dropped_frame_buffer_.get();
        if (!dropping_frame_data_){
          LOG4CXX_ERROR(logger_, "First packet from frame " << current_frame_seen_ << " detected but no free buffers available. Dropping packet data for this frame");
          dropping_frame_data_ = true;
        }
      } else {
        current_frame_buffer_id_ = empty_buffer_queue_.front();
        empty_buffer_queue_.pop();
        frame_buffer_map_[current_frame_seen_] = current_frame_buffer_id_;
        current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);

        if (!dropping_frame_data_){
          LOG4CXX_DEBUG_LEVEL(2, logger_, "First packet from frame " << current_frame_seen_ << " detected, allocating frame buffer ID " << current_frame_buffer_id_);
        } else {
          dropping_frame_data_ = false;
          LOG4CXX_DEBUG_LEVEL(2, logger_, "Free buffer now available for frame " << current_frame_seen_ << ", allocating frame buffer ID " << current_frame_buffer_id_);
        }
      }

      // Initialise frame header
      current_frame_header_ = reinterpret_cast<LATRD::FrameHeader*>(current_frame_buffer_);
      current_frame_header_->frame_number = current_frame_seen_;
      current_frame_header_->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
      current_frame_header_->packets_received = 0;
      current_frame_header_->idle_frame = 0;
      memset(current_frame_header_->packet_state, 0, LATRD::num_frame_packets);
      gettime(reinterpret_cast<struct timespec*>(&(current_frame_header_->frame_start_time)));
      if ((*(((uint64_t *)raw_packet_header())+1)&LATRD::packet_header_idle_mask) == LATRD::packet_header_idle_mask){
        LOG4CXX_DEBUG_LEVEL(2, logger_, "  IDLE packet detected");
        // Mark this buffer as a last frame buffer
        current_frame_header_->idle_frame = 1;
      }
      LOG4CXX_DEBUG_LEVEL(2, logger_, "  Initialised IDLE frame flag to: " << current_frame_header_->idle_frame);
    } else {
      current_frame_buffer_id_ = frame_buffer_map_[current_frame_seen_];
      current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
      current_frame_header_ = reinterpret_cast<LATRD::FrameHeader*>(current_frame_buffer_);
    }
  }
  // Update packet_number state map in frame header
  LOG4CXX_DEBUG_LEVEL(1, logger_, "  Setting frame " << current_frame_seen_<< " buffer ID: " << current_frame_buffer_id_ << " packet header index: " << current_frame_header_->packets_received);
  current_frame_header_->packet_state[current_frame_header_->packets_received] = 1;
}

void LATRDFrameDecoder::reset_statistics(void)
{
LOG4CXX_ERROR(logger_, "Reset statistics");
  packet_counter_ = 0;
}

void* LATRDFrameDecoder::get_next_payload_buffer() const
{
    uint8_t* next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_)
			+ get_frame_header_size()
			+ (LATRD::primary_packet_size * current_frame_header_->packets_received)
			+ LATRD::packet_header_size;

    return reinterpret_cast<void*>(next_receive_location);
}

size_t LATRDFrameDecoder::get_next_payload_size() const
{
    return LATRD::primary_packet_size;
}

FrameDecoder::FrameReceiveState LATRDFrameDecoder::process_packet(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
	// Set the frame state to incomplete for this frame
    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

    // We need to copy the 2 word packet header into the frame at the correct location
    uint8_t* packet_header_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_)
			+ get_frame_header_size()
			+ (LATRD::primary_packet_size * current_frame_header_->packets_received);
    memcpy(packet_header_location, current_raw_packet_header_.get(), LATRD::packet_header_size);

    // Increment the number of packets received for this frame
    if (!dropping_frame_data_) {
        current_frame_header_->packets_received++;
        LOG4CXX_DEBUG_LEVEL(2, logger_, "  Packet count: " << current_frame_header_->packets_received << " for frame: " << current_frame_header_->frame_number);
        LOG4CXX_DEBUG_LEVEL(2, logger_, "  IDLE frame flag: " << current_frame_header_->idle_frame);
    }
	// Check to see if the number of packets we have received is equal to the total number
	// of packets for this frame or if this is an idle frame
	if (current_frame_header_->packets_received == LATRD::num_frame_packets || current_frame_header_->idle_frame == 1){
        // If this is an idle frame then empty the current buffer map
        if (current_frame_header_->idle_frame == 1) {
            // Loop over frame buffers currently in map and flush them, not including this idle buffer as
            // that will be flushed last
            std::map<int, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
            while (buffer_map_iter != frame_buffer_map_.end()) {
                int frame_num = buffer_map_iter->first;
                int buffer_id = buffer_map_iter->second;
                void *buffer_addr = buffer_manager_->get_buffer_address(buffer_id);
                LATRD::FrameHeader *frame_header = reinterpret_cast<LATRD::FrameHeader *>(buffer_addr);
                if (current_frame_header_->frame_number != frame_num){
                    frame_header->frame_state = FrameReceiveStateComplete;
                    ready_callback_(buffer_id, frame_num);
                    frame_buffer_map_.erase(buffer_map_iter++);
                } else {
                  buffer_map_iter++;
                }
            }
        }

        // We have received the correct number of packets, the frame is complete

	    // Set frame state accordingly
		frame_state = FrameDecoder::FrameReceiveStateComplete;

		// Complete frame header
		current_frame_header_->frame_state = frame_state;

		// Check we are not dropping data for this frame
		if (!dropping_frame_data_){
			// Erase frame from buffer map
			frame_buffer_map_.erase(current_frame_seen_);

			// Notify main thread that frame is ready
			ready_callback_(current_frame_buffer_id_, current_frame_seen_);

			// Reset current frame seen ID so that if next frame has same number (e.g. repeated
			// sends of single frame 0), it is detected properly
            current_frame_seen_ = -1;
            if (current_frame_header_->idle_frame == 1) {
                current_frame_ = 1;
            } else {
                current_frame_++;
            }
		}
	}

	return frame_state;
}

//! Get the current status of the frame decoder.
//!
//! This method populates the IpcMessage passed by reference as an argument with decoder-specific
//! status information, e.g. packet loss by source.
//!
//! \param[in] param_prefix - path to be prefixed to each status parameter name
//! \param[in] status_msg - reference to IpcMesssage to be populated with parameters
//!
void LATRDFrameDecoder::get_status(const std::string param_prefix,
                                   OdinData::IpcMessage& status_msg)
{
  status_msg.set_param(param_prefix + "name", std::string("LATRDFrameDecoder"));
  status_msg.set_param(param_prefix + "packets", packet_counter_);
}

void LATRDFrameDecoder::monitor_buffers()
{

    int frames_timedout = 0;
    struct timespec current_time;

    gettime(&current_time);

    // Loop over frame buffers currently in map and check their state
    std::map<int, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
    while (buffer_map_iter != frame_buffer_map_.end())
    {
        int      frame_num = buffer_map_iter->first;
        int      buffer_id = buffer_map_iter->second;
        void*    buffer_addr = buffer_manager_->get_buffer_address(buffer_id);
        LATRD::FrameHeader* frame_header = reinterpret_cast<LATRD::FrameHeader*>(buffer_addr);

        if (elapsed_ms(frame_header->frame_start_time, current_time) > frame_timeout_ms_)
        {
            LOG4CXX_DEBUG_LEVEL(1, logger_, "Frame " << frame_num << " in buffer " << buffer_id
                    << " addr 0x" << std::hex << buffer_addr << std::dec
                    << " timed out with " << frame_header->packets_received << " packets received");

            if (current_frame_seen_ == frame_num){
              current_frame_seen_ = -1;
            }
            frame_header->frame_state = FrameReceiveStateTimedout;
            ready_callback_(buffer_id, frame_num);
            frames_timedout++;

            frame_buffer_map_.erase(buffer_map_iter++);
        }
        else
        {
            buffer_map_iter++;
        }
    }
    if (frames_timedout)
    {
        LOG4CXX_WARN(logger_, "Released " << frames_timedout << " timed out incomplete frames");
    }
    frames_timedout_ += frames_timedout;

    LOG4CXX_DEBUG_LEVEL(2, logger_, get_num_mapped_buffers() << " frame buffers in use, "
            << get_num_empty_buffers() << " empty buffers available, "
            << frames_timedout_ << " incomplete frames timed out");

}

uint8_t* LATRDFrameDecoder::raw_packet_header() const
{
    return reinterpret_cast<uint8_t*>(current_raw_packet_header_.get());
}

unsigned int LATRDFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}

/**
  * Get the plugin major version number.
  *
  * \return major version number as an integer
  */
int LATRDFrameDecoder::get_version_major()
{
    return ODIN_DATA_VERSION_MAJOR;
}

/**
  * Get the plugin minor version number.
  *
  * \return minor version number as an integer
  */
int LATRDFrameDecoder::get_version_minor()
{
    return ODIN_DATA_VERSION_MINOR;
}

/**
  * Get the plugin patch version number.
  *
  * \return patch version number as an integer
  */
int LATRDFrameDecoder::get_version_patch()
{
    return ODIN_DATA_VERSION_PATCH;
}

/**
  * Get the plugin short version (e.g. x.y.z) string.
  *
  * \return short version as a string
  */
std::string LATRDFrameDecoder::get_version_short()
{
    return ODIN_DATA_VERSION_STR_SHORT;
}

/**
  * Get the plugin long version (e.g. x.y.z-qualifier) string.
  *
  * \return long version as a string
  */
std::string LATRDFrameDecoder::get_version_long()
{
    return ODIN_DATA_VERSION_STR;
}

} /* namespace FrameReceiver */
