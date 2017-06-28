/*
 * LATRDProcessPlugin.cpp
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#include "LATRDProcessPlugin.h"

namespace FrameProcessor
{

LATRDProcessPlugin::LATRDProcessPlugin()
{
    // Setup logging for the class
    logger_ = Logger::getLogger("FW.LATRDProcessPlugin");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "LATRDProcessPlugin constructor.");
}

LATRDProcessPlugin::~LATRDProcessPlugin()
{
	// TODO Auto-generated destructor stub
}

void LATRDProcessPlugin::processFrame(boost::shared_ptr<Frame> frame)
{
	LATRD::PacketHeader packet_header;
	LOG4CXX_TRACE(logger_, "Processing raw frame.");

	// Extract the header from the buffer and print the details
	const LATRD::FrameHeader* hdrPtr = static_cast<const LATRD::FrameHeader*>(frame->get_data());
	LOG4CXX_TRACE(logger_, "Raw Frame Number: " << hdrPtr->frame_number);
	LOG4CXX_TRACE(logger_, "Frame State: " << hdrPtr->frame_state);
	LOG4CXX_TRACE(logger_, "Packets Received: " << hdrPtr->packets_received);

	// Extract the header words from each packet
	uint8_t *payload_ptr = (uint8_t*)(frame->get_data()) + sizeof(LATRD::FrameHeader);

	for (int index = 0; index < LATRD::num_primary_packets; index++){
		if (hdrPtr->packet_state[index] == 0){
			LOG4CXX_TRACE(logger_, "   Packet number: [Missing Packet]");
		} else {
			packet_header.headerWord1 = be64toh(*(uint64_t *)payload_ptr);
			packet_header.headerWord2 = be64toh(*(((uint64_t *)payload_ptr)+1));
			uint8_t *hdr_ptr = (uint8_t *)&packet_header.headerWord2;

			uint32_t packet_number = (hdr_ptr[4] << 24)
					+ (hdr_ptr[5] << 16)
					+ (hdr_ptr[6] << 8)
					+ hdr_ptr[7];

			LOG4CXX_TRACE(logger_, "   Packet number: " << packet_number);
		}
		payload_ptr += LATRD::primary_packet_size;
	}

	LOG4CXX_TRACE(logger_, "Pushing data frame.");
	this->push(frame);
}

} /* namespace filewriter */
