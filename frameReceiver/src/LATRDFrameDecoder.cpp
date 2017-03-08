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
                FrameDecoder(),
        		current_frame_seen_(-1),
        		current_frame_buffer_id_(-1),
        		current_frame_buffer_(0),
        		current_frame_header_(0),
        		dropping_frame_data_(false),
        		frame_timeout_ms_(1000),
        		frames_timedout_(0)
{
    current_packet_header_.reset(new uint8_t[sizeof(LATRD::PacketHeader)]);
    dropped_frame_buffer_.reset(new uint8_t[LATRD::total_frame_size]);
}

void LATRDFrameDecoder::init(LoggerPtr& logger, bool enable_packet_logging, unsigned int frame_timeout_ms)
{
    if (enable_packet_logging_) {
        LOG4CXX_INFO(packet_logger_, "PktHdr: SourceAddress");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               SourcePort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     DestinationPort");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      Control [63:62]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   Type [61:58]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   Producer ID [57:50]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    Time Slice [49:18]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          Spare [17:10]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    Word Count [10:0]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    |    Control [63:62]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    |    |   Type [61:58]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    |    |   |   Packet Number [57:25]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    |    |   |   |          Spare [24:0]");
        LOG4CXX_INFO(packet_logger_, "PktHdr: |               |     |      |   |   |    |          |    |    |   |   |          |        |");
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
    return sizeof(LATRD::PacketHeader);
}

void* LATRDFrameDecoder::get_packet_header_buffer(void)
{
    return current_packet_header_.get();
}

void LATRDFrameDecoder::process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr)
{
    //TODO validate header size and content, handle incoming new packet buffer allocation etc

    // Dump raw header if packet logging enabled
    if (enable_packet_logging_)
    {
        std::stringstream ss;
        uint8_t* hdr_ptr = raw_packet_header();
        ss << "PktHdr: " << std::setw(15) << std::left << inet_ntoa(from_addr->sin_addr) << std::right << " "
           << std::setw(5) << ntohs(from_addr->sin_port) << " "
           << std::setw(5) << port << std::hex << std::setfill('0');
        // Extract relevant bits into the next available size data type
        uint8_t ctrlValue = (hdr_ptr[7] & 0xC0) >> 6;
        uint8_t typeValue = (hdr_ptr[7] & 0x3C) >> 2;
        uint8_t producerID = ((hdr_ptr[7] & 0x03) << 6) + ((hdr_ptr[6] & 0xFC) >> 2);
        uint32_t timeSlice = ((hdr_ptr[6] & 0x03) << 30)
        		+ (hdr_ptr[5] << 22)
				+ (hdr_ptr[4] << 14)
				+ (hdr_ptr[3] << 6)
				+ ((hdr_ptr[2] & 0xFC) >> 2);
        uint8_t spareValue = ((hdr_ptr[2] & 0x03) << 6) + ((hdr_ptr[1] & 0xF0) >> 2);
        uint16_t wordCount = ((hdr_ptr[1] & 0x07) << 8) + hdr_ptr[0];
        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
		   << " 0x" << std::setw(2) << unsigned(producerID) << " 0x" << std::setw(8) << timeSlice << " 0x"
		   << std::setw(2) << unsigned(spareValue) << " 0x" << std::setw(4) << wordCount;

        ctrlValue = (hdr_ptr[15] & 0xC0) >> 6;
        typeValue = (hdr_ptr[15] & 0x3C) >> 2;
        uint32_t packetNumber = ((hdr_ptr[15] & 0x03) << 30)
        		+ (hdr_ptr[14] << 22)
				+ (hdr_ptr[13] << 14)
				+ (hdr_ptr[12] << 6)
				+ ((hdr_ptr[11] & 0xFC) >> 2);

        ss << " 0x" << std::setw(2) << unsigned(ctrlValue) << " 0x" << std::setw(2) << unsigned(typeValue)
		   << " 0x" << std::setw(8) << unsigned(packetNumber);

        ss << std::dec;
        LOG4CXX_INFO(packet_logger_, ss.str());
    }

	uint32_t frame = get_frame_number();
	uint16_t packet_number = get_packet_number();
	uint8_t  subframe = get_subframe_number();
	uint8_t  type = get_packet_type();

    LOG4CXX_DEBUG_LEVEL(3, logger_, "Got packet header:"
            << " type: "     << (int)type << " subframe: " << (int)subframe
            << " packet: "   << packet_number    << " frame: "    << frame
    );

    if (frame != current_frame_seen_)
    {
        current_frame_seen_ = frame;

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
            memcpy(current_frame_header_->frame_info, get_frame_info(), LATRD::frame_info_size);
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
    current_frame_header_->packet_state[type][subframe][packet_number] = 1;

}

void* LATRDFrameDecoder::get_next_payload_buffer(void) const
{

    uint8_t* next_receive_location =
            reinterpret_cast<uint8_t*>(current_frame_buffer_) +
            get_frame_header_size() +
            (LATRD::data_type_size * get_packet_type()) +
            (LATRD::subframe_size * get_subframe_number()) +
            (LATRD::primary_packet_size * get_packet_number());

    return reinterpret_cast<void*>(next_receive_location);
}

size_t LATRDFrameDecoder::get_next_payload_size(void) const
{
   size_t next_receive_size = 0;

	if (get_packet_number() < LATRD::num_primary_packets)
	{
		next_receive_size = LATRD::primary_packet_size;
	}
	else
	{
		next_receive_size = LATRD::tail_packet_size;
	}

    return next_receive_size;
}

FrameDecoder::FrameReceiveState LATRDFrameDecoder::process_packet(size_t bytes_received)
{

    FrameDecoder::FrameReceiveState frame_state = FrameDecoder::FrameReceiveStateIncomplete;

	current_frame_header_->packets_received++;

	if (current_frame_header_->packets_received == LATRD::num_frame_packets)
	{

	    // Set frame state accordingly
		frame_state = FrameDecoder::FrameReceiveStateComplete;

		// Complete frame header
		current_frame_header_->frame_state = frame_state;

		if (!dropping_frame_data_)
		{
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
    std::map<uint32_t, int>::iterator buffer_map_iter = frame_buffer_map_.begin();
    while (buffer_map_iter != frame_buffer_map_.end())
    {
        uint32_t frame_num = buffer_map_iter->first;
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

uint8_t LATRDFrameDecoder::get_packet_type(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+LATRD::packet_type_offset));
}

uint8_t LATRDFrameDecoder::get_subframe_number(void) const
{
    return *(reinterpret_cast<uint8_t*>(raw_packet_header()+LATRD::subframe_number_offset));
}

uint32_t LATRDFrameDecoder::get_frame_number(void) const
{
    uint32_t frame_number_raw = *(reinterpret_cast<uint32_t*>(raw_packet_header()+LATRD::frame_number_offset));
    return ntohl(frame_number_raw);
}

uint16_t LATRDFrameDecoder::get_packet_number(void) const
{
	uint16_t packet_number_raw = *(reinterpret_cast<uint16_t*>(raw_packet_header()+LATRD::packet_number_offset));
    return ntohs(packet_number_raw);
}

uint8_t* LATRDFrameDecoder::get_frame_info(void) const
{
    return (reinterpret_cast<uint8_t*>(raw_packet_header()+LATRD::frame_info_offset));
}

uint8_t* LATRDFrameDecoder::raw_packet_header(void) const
{
    return reinterpret_cast<uint8_t*>(current_packet_header_.get());
}


unsigned int LATRDFrameDecoder::elapsed_ms(struct timespec& start, struct timespec& end)
{

    double start_ns = ((double)start.tv_sec * 1000000000) + start.tv_nsec;
    double end_ns   = ((double)  end.tv_sec * 1000000000) +   end.tv_nsec;

    return (unsigned int)((end_ns - start_ns)/1000000);
}

} /* namespace FrameReceiver */
