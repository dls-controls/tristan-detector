/*
 * LATRDDefinitions.h
 *
 *  Created on: 23 Feb 2017
 *      Author: gnx91527
 */

#ifndef INCLUDE_LATRDDEFINITIONS_H_
#define INCLUDE_LATRDDEFINITIONS_H_

#include "gettime.h"
#include <stdint.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <time.h>

namespace LATRD
{
    static const size_t primary_packet_size    = 8192; // 1024x64bit words per packet
    static const size_t num_primary_packets    = 10;  // Number of packets in a buffer

    static const size_t packet_header_size     = 16;   // 2x64bit words in a packet header

    static const size_t number_of_processing_threads = 2;

    static const size_t frame_size = 1638400;

    typedef struct
    {
    	uint64_t headerWord1;
    	uint64_t headerWord2;
    } PacketHeader;

    typedef struct
    {
        uint32_t frame_number;
        uint32_t frame_state;
        uint8_t idle_frame;
        struct timespec frame_start_time;
        uint32_t packets_received;
        uint8_t  packet_state[num_primary_packets];
    } FrameHeader;

    static const size_t data_type_size      = primary_packet_size * num_primary_packets;
    static const size_t total_frame_size    = data_type_size + sizeof(FrameHeader);
    static const size_t num_frame_packets   = num_primary_packets;

    static const uint64_t control_word_mask             = 0x8000000000000000;
    static const uint8_t  control_type_mask             = 0x3F;
    static const uint8_t  control_header_0_mask         = 0x38;
    static const uint8_t  control_header_1_mask         = 0x39;
    static const uint8_t  control_course_timestamp_mask = 0x20;
    static const uint8_t  control_word_id_mask          = 0x0F;
    static const uint64_t control_word_idle_mask        = 0xFC00000000000000;
    static const uint64_t course_timestamp_mask         = 0x000FFFFFFFFFFFF8;
    static const uint64_t fine_timestamp_mask           = 0x0000000000FFFFFF;
    static const uint64_t energy_mask                   = 0x0000000000003FFF;
    static const uint64_t position_mask                 = 0x0000000000FFFFFF;
    static const uint64_t timestamp_match_mask          = 0x0FFFFFFF000000;
}



#endif /* INCLUDE_LATRDDEFINITIONS_H_ */
