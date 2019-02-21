//
// Created by gnx91527 on 06/08/18.
//
#include "LATRDDefinitions.h"
#include "LATRDProcessIntegral.h"
#include "LATRDExceptions.h"

namespace FrameProcessor {
  LATRDProcessIntegral::LATRDProcessIntegral() :
      width_(0),
      height_(0),
      base_image_counter_(0),
      total_count_(0),
      next_frame_id_(1),
      next_packet_id_(0)
  {
    // Setup logging for the class
    logger_ = Logger::getLogger("FP.LATRDProcessIntegral");
    logger_->setLevel(Level::getAll());
    LOG4CXX_TRACE(logger_, "LATRDProcessIntegral constructor.");

  }

  LATRDProcessIntegral::~LATRDProcessIntegral()
  {
  }

  void LATRDProcessIntegral::init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    next_frame_id_ = 1;
    next_packet_id_ = 0;
    if (image_ptr_){
      free(image_ptr_);
    }
    image_ptr_ = (uint16_t *)malloc(width_ * height_ * sizeof(uint16_t));
  }

  void LATRDProcessIntegral::reset_image()
  {
    LOG4CXX_DEBUG(logger_, "Resetting image memory");
    memset(image_ptr_, 0, (width_ * height_ * sizeof(uint16_t)));
    total_count_ = 0;
  }

  std::vector<boost::shared_ptr<Frame> > LATRDProcessIntegral::process_frame(boost::shared_ptr <Frame> frame)
  {
    std::vector<boost::shared_ptr<Frame> > image_frames;

    // Extract the header from the buffer and print the details
    const LATRD::FrameHeader *hdrPtr = static_cast<const LATRD::FrameHeader *>(frame->get_data());

    // Test for idle frames.
    if (hdrPtr->idle_frame == 1){
      LOG4CXX_DEBUG(logger_, "Count mode IDLE frame detected");

      // This is an idle frame
      // First we need to process any outstanding image frames, and then reset the image counter
      uint32_t image_counter = base_image_counter_ - 1;
      std::map<uint64_t, boost::shared_ptr<LATRDImageJob> >::iterator iter;
      for (iter = image_store_.begin(); iter != image_store_.end(); ++iter){
        image_counter++;
          if (!iter->second->get_sent()) {
            LOG4CXX_DEBUG(logger_,
                          "Creating image frame " << iter->second->get_frame_number() << " from raw buffer " << frame->get_frame_number());
            boost::shared_ptr<Frame> out_frame = iter->second->to_frame();
            image_frames.push_back(out_frame);

            iter->second->mark_sent();
          }
          iter->second->reset();
      }
      image_store_.clear();

      // and reset the image counter
      base_image_counter_ = 0;
      // and reset the expected frame ID
      next_frame_id_ = 1;

    } else {
      image_frames = frame_to_image(frame);
    }
    return image_frames;
  }

  std::vector<boost::shared_ptr<Frame> > LATRDProcessIntegral::frame_to_image(boost::shared_ptr <Frame> frame)
  {
    std::vector<boost::shared_ptr<Frame> > image_frames;
    LATRD::PacketHeader packet_header;
    // Extract the header from the buffer and print the details
    const LATRD::FrameHeader *hdrPtr = static_cast<const LATRD::FrameHeader *>(frame->get_data());

    // Extract the header words from each packet
    uint8_t *payload_ptr = (uint8_t *)(frame->get_data()) + sizeof(LATRD::FrameHeader);
    // Number of packet header 64bit words
    uint16_t packet_header_count = (LATRD::packet_header_size / sizeof(uint64_t)) - 1;

    int dropped_packets = 0;
    // Loop over each packet
    for (int index = 0; index < LATRD::num_primary_packets; index++) {
      uint32_t bad_packet = 0;
      // Ignore first header word as it is not used.
      packet_header.headerWord1 = *(((uint64_t *) payload_ptr) + 1);
      packet_header.headerWord2 = *(((uint64_t *) payload_ptr) + 2);

      if (hdrPtr->packet_state[index] == 0) {
        dropped_packets += 1;
      } else {
        // Walk through each data word
        uint16_t word_count = LATRD::get_word_count(packet_header.headerWord1);
        uint32_t packet_id = LATRD::get_packet_number(packet_header.headerWord2);
        uint32_t image_number = LATRD::get_image_number(packet_header.headerWord2);
        // Ignore the first 0x00000000 which is not used
        uint64_t *data_word_ptr = (((uint64_t *) payload_ptr) + 1 + packet_header_count);

        uint64_t packet_timestamp = 0;
        try {
          packet_timestamp = get_course_timestamp(*data_word_ptr);
        } catch (...){
          // Caught an exception whilst reading the course timestamp so dump the packet header
          LOG4CXX_ERROR(logger_, "Caught ERROR whilst decoding a count mode packet: Could not retrieve course timestamp");
          std::stringstream ss1;
          ss1 << std::hex << std::setfill('0');
          ss1 << " 0x" << std::setw(16) << packet_header.headerWord1;
          LOG4CXX_ERROR(logger_, "Header 1: " << ss1.str());
          std::stringstream ss2;
          ss2 << std::hex << std::setfill('0');
          ss2 << " 0x" << std::setw(16) << packet_header.headerWord2;
          LOG4CXX_ERROR(logger_, "Header 2: " << ss2.str());
          std::stringstream ss3;
          ss3 << std::hex << std::setfill('0');
          ss3 << " 0x" << std::setw(16) << *data_word_ptr;
          LOG4CXX_ERROR(logger_, "Tmstamp : " << ss3.str());
          bad_packet = 1;
        }

        if (!bad_packet) {
          LOG4CXX_DEBUG(logger_,
                        "Pkt [" << packet_id << "] timestamp [" << packet_timestamp << "] word count " << word_count);
          data_word_ptr++;

          // Check if we have an image job for this packet's timestamp
          boost::shared_ptr<LATRDImageJob> image_job_ptr;
          if (image_store_.count(packet_timestamp) > 0) {
            image_job_ptr = image_store_[packet_timestamp];
          } else {
            // We need to create a new image job for this packet
            // TODO: Check this is not an old packet
            image_job_ptr = boost::shared_ptr<LATRDImageJob>(new LATRDImageJob(width_, height_, image_number));
            // Store the image job in the store, index by timestamp
            image_store_[packet_timestamp] = image_job_ptr;
          }

          // Start from index 3 as we can ignore the header words and extended timestamp
          for (uint16_t index = 3; index < word_count; index++) {
            uint32_t x_pos = 0;
            uint32_t y_pos = 0;
            uint32_t i_tot = 0;
            uint32_t event_count = 0;
            uint32_t e_tot = 0;
            try {
              // Check if the word is a final packet word
              if (check_for_final_packet_word(*data_word_ptr)) {
                LOG4CXX_DEBUG(logger_, "Image [" << image_job_ptr->get_frame_number() << "] End Of Image on packet ["
                                                 << packet_id << "]");
                image_job_ptr->set_eoi(packet_id);
              } else {
                if (process_data_word(*data_word_ptr,
                                      &x_pos,
                                      &y_pos,
                                      &i_tot,
                                      &event_count)) {
                  // Add the event count to the 2D image
                  image_job_ptr->add_pixel(packet_id, x_pos, y_pos, event_count);
                  total_count_ += event_count;
                }
              }
            }
            catch (LATRDProcessingException &ex) {
              // TODO: What to do here if there is an exception
            }
            data_word_ptr++;
          }
        }
      }
      // Increment the payload pointer to the next packet
      payload_ptr += LATRD::primary_packet_size;
    }
    // After processing all of the packets, loop through the job map and see if we can pass out any frames
    uint32_t image_counter = base_image_counter_ - 1;
    std::vector<uint64_t> delete_image_vector;
    std::map<uint64_t, boost::shared_ptr<LATRDImageJob> >::iterator iter;
    for (iter = image_store_.begin(); iter != image_store_.end(); ++iter){
        image_counter++;
        if (iter->second->verify_image()){
          if (!iter->second->get_sent()) {
            LOG4CXX_DEBUG(logger_,
                          "Creating image frame " << iter->second->get_frame_number() << " from raw buffer " << frame->get_frame_number());
            boost::shared_ptr<Frame> out_frame = iter->second->to_frame();
            image_frames.push_back(out_frame);

            iter->second->mark_sent();
          }
          // Mark the frame for deletion
          delete_image_vector.push_back(iter->first);
        }
    }
    std::vector<uint64_t>::iterator del_iter;
    for (del_iter = delete_image_vector.begin(); del_iter != delete_image_vector.end(); ++del_iter){
        if (image_store_.begin()->first == *del_iter){
            // Reset the job
            image_store_[*del_iter]->reset();
            // Delete the job
            image_store_.erase(*del_iter);
            // Increment the base
            base_image_counter_++;
        }
    }


    return image_frames;
  }

  bool LATRDProcessIntegral::process_data_word(uint64_t data_word,
                                               uint32_t *chip_x,
                                               uint32_t *chip_y,
                                               uint32_t *i_tot,
                                               uint32_t *event_count)
  {
    if (check_for_integral_data_word(data_word)){
      // Extract the information from the data word
      *chip_x = (uint32_t)((data_word&integral_chip_x_mask)>>37);
      *chip_y = (uint32_t)((data_word&integral_chip_y_mask)>>24);
      *i_tot = (uint32_t)((data_word&integral_i_tot_mask)>>10);
      *event_count = (uint32_t)(data_word&integral_evt_count_mask);
      return true;
    }
    return false;
  }

  bool LATRDProcessIntegral::process_control_word(uint64_t ctrl_word)
  {
    return false;
  }

  uint64_t LATRDProcessIntegral::get_course_timestamp(uint64_t data_word)
  {
    if (!LATRD::is_control_word(data_word)){
      throw LATRDProcessingException("Data word is not a control word");
    }
    return data_word & LATRD::course_timestamp_mask;
  }

  bool LATRDProcessIntegral::check_for_final_packet_word(uint64_t data_word)
  {
    if ((data_word & integral_final_packet_mask) == integral_final_packet_value){
      return true;
    }
    return false;
  }

  bool LATRDProcessIntegral::check_for_integral_data_word(uint64_t data_word)
  {
    if ((data_word & integral_data_word_mask) == integral_data_word_value){
      return true;
    }
    return false;
  }

}