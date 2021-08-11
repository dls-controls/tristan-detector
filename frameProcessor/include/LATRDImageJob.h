//
// Created by gnx91527 on 27/09/18.
//

#ifndef LATRD_LATRDIMAGEJOB_H
#define LATRD_LATRDIMAGEJOB_H

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Frame.h"

namespace FrameProcessor {

    class LATRDImageJob
    {
    public:
      LATRDImageJob(uint32_t width, uint32_t height, uint32_t number);
      virtual ~LATRDImageJob();
      void set_eoi(uint32_t packet_id);
      uint32_t get_frame_number();
      void add_pixel(uint32_t packet_id, uint32_t x, uint32_t y, uint32_t event_count);
      bool verify_image();
      boost::shared_ptr<Frame> to_frame(uint32_t multiplier, int32_t offset);
      void reset();
      void mark_sent();
      bool get_sent();

      uint32_t width_;
      uint32_t height_;
      uint32_t frame_number_;
      uint16_t *image_ptr_;
      uint64_t timestamp_;
      bool sent_;
      std::map<uint32_t, uint32_t> packet_ids_;
      int32_t eoi_packet_id_;
    };

} /* namespace FrameProcessor */

#endif //LATRD_LATRDIMAGEJOB_H
