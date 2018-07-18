/*
 * LATRDFrameDecoder.h
 *
 *  Created on: 23 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_LATRDFRAMEDECODER_H_
#define SRC_LATRDFRAMEDECODER_H_

#include "FrameDecoderUDP.h"
#include "LATRDDefinitions.h"
#include "gettime.h"
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>
#include <arpa/inet.h>
#include <boost/format.hpp>

namespace FrameReceiver
{

class LATRDFrameDecoder : public FrameDecoderUDP
{
public:
	LATRDFrameDecoder();
	virtual ~LATRDFrameDecoder();
	void init(LoggerPtr& logger, OdinData::IpcMessage& config_msg);
    const size_t get_frame_buffer_size(void) const;
    const size_t get_frame_header_size(void) const;

    inline const bool requires_header_peek(void) const { return true; };
    const size_t get_packet_header_size(void) const;
    void process_packet_header(size_t bytes_received, int port, struct sockaddr_in* from_addr);

    void* get_next_payload_buffer(void) const;
    size_t get_next_payload_size(void) const;
    FrameDecoder::FrameReceiveState process_packet(size_t bytes_received);

    void monitor_buffers(void);
    void get_status(const std::string param_prefix, OdinData::IpcMessage& status_msg);

    void* get_packet_header_buffer(void);

    uint32_t get_frame_number(void) const;
//    uint32_t get_packet_number(void) const;
    uint32_t get_frame_packet_number(void) const;
//    uint8_t get_producer_ID(void) const;
//    uint32_t get_time_slice(void) const;
//    uint16_t get_word_count(void) const;

private:

    uint8_t* raw_packet_header(void) const;
    unsigned int elapsed_ms(struct timespec& start, struct timespec& end);

    boost::shared_ptr<void> current_raw_packet_header_;
    boost::shared_ptr<void> dropped_frame_buffer_;

    uint32_t current_frame_seen_;
    int current_frame_buffer_id_;
    void* current_frame_buffer_;
    LATRD::PacketHeader current_packet_header_;
    LATRD::FrameHeader* current_frame_header_;

    bool dropping_frame_data_;

    unsigned int frame_timeout_ms_;
    unsigned int frames_timedout_;
};

} /* namespace FrameReceiver */

#endif /* SRC_LATRDFRAMEDECODER_H_ */
