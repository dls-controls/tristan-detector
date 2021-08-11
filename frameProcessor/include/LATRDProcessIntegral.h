//
// Created by gnx91527 on 06/08/18.
//

#ifndef LATRD_LATRDPROCESSINTEGRAL_H
#define LATRD_LATRDPROCESSINTEGRAL_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>

#include <stdlib.h>
#include <stdint.h>
#include <vector>
#include <map>

#include "Frame.h"
#include "LATRDImageJob.h"

namespace FrameProcessor {

  static const uint64_t integral_data_word_mask       = 0xFC00000000000000;
  static const uint64_t integral_data_word_value      = 0x9000000000000000;

  static const uint64_t integral_chip_x_mask          = 0x0003FFE000000000;
  static const uint64_t integral_chip_y_mask          = 0x0000001FFF000000;
  static const uint64_t integral_i_tot_mask           = 0x0000000000FFFC00;
  static const uint64_t integral_evt_count_mask       = 0x00000000000003FF;

  static const uint64_t integral_final_packet_mask    = 0xF000FFFF00000000;
  static const uint64_t integral_final_packet_value   = 0xC00071B000000000;

  class LATRDProcessIntegral
  {
  public:
    LATRDProcessIntegral();
    virtual ~LATRDProcessIntegral();
    void init(uint32_t width, uint32_t height, uint32_t offset_x, uint32_t offset_y);
    void reset_image();
    void configure_processes(uint32_t rank, uint32_t processes, uint32_t fpps);
    std::vector<boost::shared_ptr<Frame> > process_frame(boost::shared_ptr<Frame> frame);
    std::vector<boost::shared_ptr<Frame> > frame_to_image(boost::shared_ptr <Frame> frame);
    bool process_data_word(uint64_t data_word,
                           uint32_t *chip_x,
                           uint32_t *chip_y,
                           uint32_t *i_tot,
                           uint32_t *event_count);
    bool process_control_word(uint64_t ctrl_word);
    uint64_t get_course_timestamp(uint64_t data_word);
    bool check_for_final_packet_word(uint64_t data_word);
    bool check_for_integral_data_word(uint64_t data_word);

  private:
    /** Pointer to logger */
    LoggerPtr logger_;

    uint32_t width_;
    uint32_t height_;
    uint32_t offset_x_;
    uint32_t offset_y_;
    int32_t frame_offset_;
    uint32_t frame_multiplier_;
    uint32_t base_image_counter_;
    uint32_t total_count_;
    uint32_t next_frame_id_;
    uint32_t next_packet_id_;
    uint16_t *image_ptr_;
    std::map<uint32_t, boost::shared_ptr<Frame> > frame_store_;
    std::map<uint64_t, boost::shared_ptr<LATRDImageJob> > image_store_;
  };


}

#endif //LATRD_LATRDPROCESSINTEGRAL_H
