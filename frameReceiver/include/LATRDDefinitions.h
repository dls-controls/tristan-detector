/*
 * LATRDDefinitions.h
 *
 *  Created on: 23 Feb 2017
 *      Author: gnx91527
 */

#ifndef INCLUDE_LATRDDEFINITIONS_H_
#define INCLUDE_LATRDDEFINITIONS_H_

namespace LATRD
{
    static const size_t primary_packet_size    = 8192; // 1024x64bit words per packet
    static const size_t num_primary_packets    = 10;  // Number of packets in a buffer

//    static const size_t packet_header_size     = 16;   // 2x64bit words in a packet header
    // TODO: This is a fudge because currently packets are arriving with the first
    // 64 bits all set to 1.  Ignore this word.
    static const size_t packet_header_size     = 24;   // 2x64bit words in a packet header

    typedef struct
    {
    	uint64_t headerWord1;
    	uint64_t headerWord2;
    } PacketHeader;

    typedef struct
    {
        uint32_t frame_number;
        uint32_t frame_state;
        struct timespec frame_start_time;
        uint32_t packets_received;
        uint8_t  packet_state[num_primary_packets];
    } FrameHeader;

    static const size_t data_type_size      = primary_packet_size * num_primary_packets;
    static const size_t total_frame_size    = data_type_size + sizeof(FrameHeader);
    static const size_t num_frame_packets   = num_primary_packets;

}



#endif /* INCLUDE_LATRDDEFINITIONS_H_ */
