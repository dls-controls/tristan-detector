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

	// Number of packet header 64bit words
	uint16_t packet_header_count = LATRD::packet_header_size/sizeof(uint64_t);

	for (int index = 0; index < LATRD::num_primary_packets; index++){
		if (hdrPtr->packet_state[index] == 0){
			LOG4CXX_TRACE(logger_, "   Packet number: [Missing Packet]");
		} else {
			packet_header.headerWord1 = be64toh(*(uint64_t *)payload_ptr);
			packet_header.headerWord2 = be64toh(*(((uint64_t *)payload_ptr)+1));

			// We need to decode how many values are in the packet
			uint32_t packet_number = get_packet_number(&packet_header.headerWord2);
			uint16_t word_count = get_word_count(&packet_header.headerWord1);

			LOG4CXX_TRACE(logger_, "   Packet number: " << packet_number);

			LOG4CXX_TRACE(logger_, "   Word count: " << word_count);

			uint16_t words_to_process = word_count - packet_header_count;
			// Loop over words to process
			uint64_t *data_ptr = (uint64_t *)payload_ptr;
			data_ptr += packet_header_count;
			for (uint16_t index = 0; index < words_to_process; index++){
				uint64_t data_word = be64toh(*data_ptr);
				processDataWord(data_word);
			}
		}
		payload_ptr += LATRD::primary_packet_size;
	}

	LOG4CXX_TRACE(logger_, "Pushing data frame.");
	std::vector<dimsize_t> dims;
	dims.push_back(10240);
	dims.push_back(1);
	frame->set_dataset_name("frame");
	frame->set_data_type(3);
	frame->set_dimensions("frame", dims);
	this->push(frame);
}

void LATRDProcessPlugin::processDataWord(uint64_t data_word)
{

}

uint32_t LATRDProcessPlugin::get_packet_number(void *headerWord2) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)headerWord2;

	return (hdr_ptr[4] << 24)
			+ (hdr_ptr[5] << 16)
			+ (hdr_ptr[6] << 8)
			+ hdr_ptr[7];
}

uint16_t LATRDProcessPlugin::get_word_count(void *headerWord1) const
{
	// Read the header word as bytes
    uint8_t *hdr_ptr = (uint8_t *)headerWord1;

    // Extract relevant bits to obtain the word count
    return ((hdr_ptr[6] & 0x07) << 8) + hdr_ptr[7];
}


} /* namespace filewriter */
