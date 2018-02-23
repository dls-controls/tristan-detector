/*
 * LATRDFrameDecoder.cpp
 *
 *  Created on: 23 Feb 2017
 *      Author: gnx91527
 */

#include "LATRDFrameDecoder.h"

namespace FrameReceiver
{

LATRDFrameDecoder::LATRDFrameDecoder() :
                FrameDecoderUDP(),
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

const size_t LATRDFrameDecoder::get_frame_buffer_size(void) const
{
    return LATRD::total_frame_size;
}

const size_t LATRDFrameDecoder::get_frame_header_size(void) const
{
    return sizeof(LATRD::FrameHeader);
}

const size_t LATRDFrameDecoder::get_packet_header_size(void) const
{
    return LATRD::packet_header_size;
}

void* LATRDFrameDecoder::get_packet_header_buffer(void)
{
    return current_raw_packet_header_.get();
}

void LATRDFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    //TODO validate header size and content, handle incoming new packet buffer allocation etc

  // Convert both header 64bit values to host endian
//  current_packet_header_.headerWord1 = be64toh(*(uint64_t *)raw_packet_header());
//  current_packet_header_.headerWord2 = be64toh(*(((uint64_t *)raw_packet_header())+1));

  // TODO: This is a fudge because currently packets are arriving with the first
  // 64 bits all set to 1.  Ignore this word.
  current_packet_header_.headerWord1 = be64toh(*(((uint64_t *)raw_packet_header())+1));
  current_packet_header_.headerWord2 = be64toh(*(((uint64_t *)raw_packet_header())+2));

	// Read the decoded header information into local variables for use
	uint16_t packetNumber = get_packet_number();
	uint8_t producerID = get_producer_ID();
	uint32_t timeSlice = get_time_slice();
	uint16_t wordCount = get_word_count();
	uint32_t frameNumber = get_frame_number();
	uint32_t framePacketNumber = get_frame_packet_number();

    // Dump raw header if packet logging enabled
    if (enable_packet_logging_)
    {
        std::stringstream ss;
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex << std::setfill('0');

        // Obtain a pointer to the first 64 bit header word
        uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord1;

        // Extract relevant bits into the next available size data type
        uint8_t ctrlValue = (hdr_ptr[0] & 0xC0) >> 6;
        uint8_t typeValue = (hdr_ptr[0] & 0x3C) >> 2;
        uint8_t spareValue = ((hdr_ptr[5] & 0x03) << 6) + ((hdr_ptr[6] & 0xF0) >> 2);

        // Format the data values into the stream
        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
		   << " 0x" << std::setw(2) << unsigned(producerID) << " 0x" << std::setw(8) << timeSlice << " 0x"
		   << std::setw(2) << unsigned(spareValue) << " 0x" << std::setw(4) << wordCount;

        // Obtain a pointer to the second 64 bit header word
        hdr_ptr = (uint8_t *)&current_packet_header_.headerWord2;

        ctrlValue = (hdr_ptr[0] & 0xC0) >> 6;
        typeValue = (hdr_ptr[0] & 0x3C) >> 2;
        spareValue = ((hdr_ptr[0] & 0x03) << 24)
                	+ (hdr_ptr[1] << 16)
                	+ (hdr_ptr[2] << 8)
                	+ (hdr_ptr[3]);

        // Format the data values into the stream
        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
		<< " 0x" << std::setw(8) << unsigned(spareValue) << " 0x" << std::setw(8) << unsigned(packetNumber);

        ss << std::dec;
        LOG4CXX_INFO(packet_logger_, ss.str());
    }

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Got packet header:" << " packet: " << packetNumber);
    LOG4CXX_DEBUG_LEVEL(3, logger_, "Frame packet number: " << framePacketNumber << " frame: " << frameNumber);

    if (frameNumber != current_frame_seen_)
    {
        current_frame_seen_ = frameNumber;

    	if (frame_buffer_map_.count(current_frame_seen_) == 0)
    	{
    	    if (empty_buffer_queue_.empty())
            {
                current_frame_buffer_ = dropped_frame_buffer_.get();

    	        if (!dropping_frame_data_)
                {
                    LOG4CXX_ERROR(logger_, "First packet from frame " << current_frame_seen_ << " detected but no free buffers available. Dropping packet data for this frame");
                    dropping_frame_data_ = true;
                }
            }
    	    else
    	    {

                current_frame_buffer_id_ = empty_buffer_queue_.front();
                empty_buffer_queue_.pop();
                frame_buffer_map_[current_frame_seen_] = current_frame_buffer_id_;
                current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);

                if (!dropping_frame_data_)
                {
                    LOG4CXX_DEBUG_LEVEL(2, logger_, "First packet from frame " << current_frame_seen_ << " detected, allocating frame buffer ID " << current_frame_buffer_id_);
                }
                else
                {
                    dropping_frame_data_ = false;
                    LOG4CXX_DEBUG_LEVEL(2, logger_, "Free buffer now available for frame " << current_frame_seen_ << ", allocating frame buffer ID " << current_frame_buffer_id_);
                }
    	    }

    	    // Initialise frame header
            current_frame_header_ = reinterpret_cast<LATRD::FrameHeader*>(current_frame_buffer_);
            current_frame_header_->frame_number = current_frame_seen_;
            current_frame_header_->frame_state = FrameDecoder::FrameReceiveStateIncomplete;
            current_frame_header_->packets_received = 0;
            memset(current_frame_header_->packet_state, 0, LATRD::num_frame_packets);
            gettime(reinterpret_cast<struct timespec*>(&(current_frame_header_->frame_start_time)));

    	}
    	else
    	{
    		current_frame_buffer_id_ = frame_buffer_map_[current_frame_seen_];
        	current_frame_buffer_ = buffer_manager_->get_buffer_address(current_frame_buffer_id_);
        	current_frame_header_ = reinterpret_cast<LATRD::FrameHeader*>(current_frame_buffer_);
    	}

    }

    // Update packet_number state map in frame header
    current_frame_header_->packet_state[framePacketNumber] = 1;

}

void* LATRDFrameDecoder::get_next_payload_buffer(void) const
{
    uint8_t* next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_)
			+ get_frame_header_size()
			+ (LATRD::primary_packet_size * get_frame_packet_number())
			+ LATRD::packet_header_size;

    return reinterpret_cast<void*>(next_receive_location);
}

size_t LATRDFrameDecoder::get_next_payload_size(void) const
{
    return LATRD::primary_packet_size;
}

FrameDecoder::FrameReceiveState LATRDFrameDecoder::process_packet(size_t bytes_received)
{
	// Set the frame state to incomplete for this frame
    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

    // We need to copy the 2 word packet header into the frame at the correct location
    uint8_t* packet_header_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_)
			+ get_frame_header_size()
			+ (LATRD::primary_packet_size * get_frame_packet_number());
    memcpy(packet_header_location, current_raw_packet_header_.get(), sizeof(uint64_t)*2);

    // Increment the number of packets received for this frame
	current_frame_header_->packets_received++;

	// Check to see if the number of packets we have received is equal to the total number
	// of packets for this frame
	if (current_frame_header_->packets_received == LATRD::num_frame_packets){
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
		}
	}

	return frame_state;
}

void LATRDFrameDecoder::monitor_buffers(void)
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

uint32_t LATRDFrameDecoder::get_frame_number(void) const
{
	// Frame number is derived from the packet number and the number of packets in a frame
	// Frame number starts at 1
    uint32_t frame_number = get_packet_number() / LATRD::num_primary_packets;
    return frame_number;
}

uint32_t LATRDFrameDecoder::get_packet_number(void) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord2;

	return (hdr_ptr[4] << 24)
			+ (hdr_ptr[5] << 16)
			+ (hdr_ptr[6] << 8)
			+ hdr_ptr[7];
}

uint32_t LATRDFrameDecoder::get_frame_packet_number(void) const
{
	return get_packet_number() % LATRD::num_primary_packets;
}

uint8_t LATRDFrameDecoder::get_producer_ID(void) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord1;

    // Extract relevant bits to obtain the producer ID
    return ((hdr_ptr[0] & 0x03) << 6) + ((hdr_ptr[1] & 0xFC) >> 2);
}

uint32_t LATRDFrameDecoder::get_time_slice(void) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord1;

    // Extract relevant bits to obtain the time slice
    return ((hdr_ptr[1] & 0x03) << 30)
    		+ (hdr_ptr[2] << 22)
			+ (hdr_ptr[3] << 14)
			+ (hdr_ptr[4] << 6)
			+ ((hdr_ptr[5] & 0xFC) >> 2);
}

uint16_t LATRDFrameDecoder::get_word_count(void) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)&current_packet_header_.headerWord1;

    // Extract relevant bits to obtain the word count
    return ((hdr_ptr[6] & 0x07) << 8) + hdr_ptr[7];
}

uint8_t* LATRDFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_raw_packet_header_.get());
}


unsigned int LATRDFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}

} /* namespace FrameReceiver */
