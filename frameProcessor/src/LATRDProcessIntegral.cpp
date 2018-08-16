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
      image_counter_(0),
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
      std::map<uint32_t, boost::shared_ptr<Frame> >::iterator iter;
      for (iter = frame_store_.begin(); iter != frame_store_.end(); ++iter) {
        std::vector <boost::shared_ptr<Frame> > frames = frame_to_image(iter->second);
        image_frames.insert(image_frames.end(), frames.begin(), frames.end());
      }
      // Now empty the frame store
      frame_store_.clear();
      // Reset the image
      reset_image();
      // and reset the image counter
      image_counter_ = 0;
      // and reset the expected frame ID
      next_frame_id_ = 1;
      // and reset the expected packet ID
      next_packet_id_ = 0;

    } else {
      // Check packet ID against latest.
      if (hdrPtr->first_packet == next_packet_id_) {
        // If first packet number is the next one then process it immediately
        // Process frame
        image_frames = frame_to_image(frame);
        // Increment next packet counter
        next_packet_id_ = hdrPtr->last_packet + 1;
      } else {
        if (hdrPtr->first_packet > next_packet_id_) {
          // If not then store it
          frame_store_[hdrPtr->first_packet] = frame;
        } else {
          // We have received a frame with a number that we have already processed
          // This could be a late packet.
          // Log the error
          LOG4CXX_DEBUG(logger_, "Received old buffer number : " << frame->get_frame_number() << " with packet ID: " << hdrPtr->first_packet);
        }
      }
      // Check if we can process further stored frames in order
      while (frame_store_.count(next_packet_id_) > 0) {
        // Process frame
        std::vector <boost::shared_ptr<Frame> > frames = frame_to_image(frame_store_[next_packet_id_]);
        const LATRD::FrameHeader *fHdrPtr = static_cast<const LATRD::FrameHeader *>(frame_store_[next_packet_id_]->get_data());
        uint32_t new_packet_id = fHdrPtr->last_packet + 1;
        image_frames.insert(image_frames.end(), frames.begin(), frames.end());
        // Remove frame from store
        frame_store_.erase(next_packet_id_);
        // Increment next frame counter
        next_packet_id_ = new_packet_id;
      }
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
      // Ignore first header word as it is not used.
      packet_header.headerWord1 = *(((uint64_t *) payload_ptr) + 1);
      packet_header.headerWord2 = *(((uint64_t *) payload_ptr) + 2);

      if (hdrPtr->packet_state[index] == 0) {
        dropped_packets += 1;
      } else {
        // Walk through each data word
        uint16_t word_count = LATRD::get_word_count(packet_header.headerWord1);
        uint32_t packet_id = LATRD::get_packet_number(packet_header.headerWord2);
        // Ignore the extended timestamp and the first 0x00000000 which is not used
        uint64_t *data_word_ptr = (((uint64_t *) payload_ptr) + 2 + packet_header_count);
        LOG4CXX_DEBUG(logger_, "Pkt [" << packet_id << "] word count " << word_count);

        // Start from index 3 as we can ignore the header words and extended timestamp
        for (uint16_t index = 3; index < word_count; index++) {
          uint32_t x_pos = 0;
          uint32_t y_pos = 0;
          uint32_t i_tot = 0;
          uint32_t event_count = 0;
          uint32_t e_tot = 0;
          try {
            if (check_for_final_packet_word(*data_word_ptr)) {
              // Check if the word is a final packet word
              LOG4CXX_DEBUG(logger_, "Pkt [" << packet_id << "] Creating image frame " << image_counter_ << " from raw buffer " << frame->get_frame_number());
              //LOG4CXX_DEBUG(logger_, "  =>  Image total count " << total_count_);

              // Create the frame object to wrap the image
              boost::shared_ptr<Frame> out_frame = boost::shared_ptr<Frame>(new Frame("image"));
              out_frame->copy_data(image_ptr_, width_ * height_ * sizeof(uint16_t));
              out_frame->set_frame_number(image_counter_);
              std::vector<dimsize_t> dims(0);
              dims.push_back(height_);
              dims.push_back(width_);
              out_frame->set_dataset_name("image");
              out_frame->set_data_type(1);
              out_frame->set_dimensions("image", dims);
              // Reset the image
              reset_image();
              // Increase the image counter
              image_counter_++;
              // Add the frame to the return vector
              image_frames.push_back(out_frame);
            } else {
              if (process_data_word(*data_word_ptr,
                                    &x_pos,
                                    &y_pos,
                                    &i_tot,
                                    &event_count)) {
                // Add the event count to the 2D image
                uint32_t data_index = x_pos + (y_pos * width_);
                image_ptr_[data_index] += event_count;
                //LOG4CXX_DEBUG(logger_, "Pkt [" << packet_id << "] writing " << event_count << " counts [" << x_pos << "," << y_pos << "]");
                total_count_ += event_count;
              }
            }
          }
          catch (LATRDProcessingException &ex){
            // TODO: What to do here if there is an exception
          }
          data_word_ptr++;
        }
      }
      // Increment the payload pointer to the next packet
      payload_ptr += LATRD::primary_packet_size;
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