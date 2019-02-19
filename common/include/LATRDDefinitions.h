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
  static const size_t num_primary_packets    = 100;  // Number of packets in a buffer

  // static const size_t packet_header_size     = 16;   // 2x64bit words in a packet header
  // TODO: This is a fudge because currently packets are arriving with the first
  // 64 bits all set to 1.  Ignore this word.
  static const size_t packet_header_size     = 24;   // 2x64bit words in a packet header

  static const size_t number_of_processing_threads = 8;

  static const size_t number_of_time_slice_buffers = 4;

  static const size_t frame_size = 524288;  // 4MB / 8 byte values

  enum LATRDDataControlType {
    Unknown, HeaderWord0, HeaderWord1, ExtendedTimestamp, IdleControlWord
  };

  typedef struct
  {
  	uint64_t headerWord1;
  	uint64_t headerWord2;
  } PacketHeader;

  typedef struct
  {
    uint32_t frame_number;
    uint32_t frame_state;
    uint32_t idle_frame;
    struct timespec frame_start_time;
    uint32_t packets_received;
    uint8_t  packet_state[num_primary_packets];
  } FrameHeader;

  static const size_t data_type_size      = primary_packet_size * num_primary_packets;
  static const size_t total_frame_size    = data_type_size + sizeof(FrameHeader);
  static const size_t num_frame_packets   = num_primary_packets;

  static const uint64_t packet_header_idle_mask         = 0x000000000003F800;
  static const uint64_t control_word_mask               = 0x8000000000000000;
  static const uint8_t  control_type_mask               = 0x3F;
  static const uint8_t  control_header_0_mask           = 0x38;
  static const uint8_t  control_header_1_mask           = 0x39;
  static const uint8_t  control_course_timestamp_mask   = 0x20;
  static const uint8_t  control_word_id_mask            = 0x0F;
  static const uint64_t control_word_full_mask          = 0xFFF0000000000000;
  static const uint64_t header_packet_producer_mask     = 0x03FC000000000000;
  static const uint64_t header_packet_count_mask        = 0x00000000FFFFFFFF;
  static const uint64_t header_packet_ts_number_mask    = 0x000000FF00000000;
  static const uint64_t header_packet_image_number_mask = 0x00FFFFFF00000000;
  static const uint64_t control_word_count_mask         = 0x00000000000007FF;
  static const uint64_t control_word_idle_mask          = 0xFC00000000000000;
  static const uint64_t control_word_time_slice_mask    = 0x0003FFFFFFFC0000;
  static const uint64_t course_timestamp_mask           = 0x000FFFFFFFFFFFF8;
  static const uint64_t fine_timestamp_mask             = 0x00000000007FFFFF;
  static const uint64_t energy_mask                     = 0x0000000000003FFF;
  static const uint64_t position_mask                   = 0x0000000003FFFFFF;
  static const uint64_t timestamp_match_mask            = 0x000FFFFFFF800000;

  static const uint64_t course_timestamp_rollover       = 0x00000000001FFFFF;

  static uint8_t get_producer_ID(uint64_t  headerWord1)
  {
    // Extract relevant bits to obtain the producer ID
    return (uint8_t)((headerWord1 & header_packet_producer_mask) >> 50);
  }

  static uint16_t get_word_count(uint64_t headerWord1)
  {
    // Extract relevant bits to obtain the word count
    return (uint16_t )(headerWord1 & control_word_count_mask);
  }

  static uint32_t get_time_slice_modulo(uint64_t headerWord1)
  {
    // Extract relevant bits to obtain the time slice ID
    return (uint32_t )((headerWord1 & control_word_time_slice_mask) >> 18);
  }

  static uint8_t get_time_slice_number(uint64_t headerWord2)
  {
    // Extract relevant bits to obtain the time slice number
    return (uint8_t )((headerWord2 & header_packet_ts_number_mask) >> 32);
  }

  static uint8_t get_image_number(uint64_t headerWord2)
  {
    // Extract relevant bits to obtain the time slice number
    return (uint32_t )((headerWord2 & header_packet_image_number_mask) >> 32);
  }

  static uint32_t get_time_slice_id(uint64_t headerWord1, uint64_t headerWord2)
  {
    // The time slice ID is calculated by multiplying the modulo by the number of buffers
    // and then adding the current buffer number
    return (get_time_slice_modulo(headerWord1) * number_of_time_slice_buffers) + get_time_slice_number(headerWord2);
  }

  static uint32_t get_packet_number(uint64_t headerWord2)
  {
    // Extract relevant bits to obtain the packet count
    return (uint32_t )(headerWord2 & header_packet_count_mask);
  }

  static bool is_control_word(uint64_t data_word)
  {
    bool ctrlWord = false;
    if ((data_word & LATRD::control_word_mask) > 0){
      ctrlWord = true;
    }
    return ctrlWord;
  }

  static LATRDDataControlType get_control_type(uint64_t data_word)
  {
    LATRDDataControlType ctrl_type = Unknown;
    if (is_control_word(data_word)) {
      uint8_t ctrl_word = (data_word >> 58) & LATRD::control_type_mask;
      if (ctrl_word == LATRD::control_course_timestamp_mask) {
        ctrl_type = ExtendedTimestamp;
      } else if (ctrl_word == LATRD::control_header_0_mask) {
        ctrl_type = HeaderWord0;
      } else if (ctrl_word == LATRD::control_header_1_mask) {
        ctrl_type = HeaderWord1;
      }
    }
    return ctrl_type;
  }


}



#endif /* INCLUDE_LATRDDEFINITIONS_H_ */
