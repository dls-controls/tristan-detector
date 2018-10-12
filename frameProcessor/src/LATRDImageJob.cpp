//
// Created by gnx91527 on 27/09/18.
//
#include "LATRDImageJob.h"

namespace FrameProcessor {

    LATRDImageJob::LATRDImageJob(uint32_t width, uint32_t height)
    {
      width_ = width;
      height_ = height;
      image_ptr_ = (uint16_t *)malloc(width * height * sizeof(uint16_t));
      eoi_packet_id_ = -1;
      sent_ = false;
      this->reset();
    }

    LATRDImageJob::~LATRDImageJob()
    {
      free(image_ptr_);
    }

    void LATRDImageJob::set_eoi(uint32_t packet_id)
    {
        eoi_packet_id_ = (int32_t)packet_id;
    }

    void LATRDImageJob::add_pixel(uint32_t packet_id, uint32_t x, uint32_t y, uint32_t event_count)
    {
      // Calculate the data index
      uint32_t data_index = x + (y * width_);
      // Record the event count into the correct pixel
//      printf("Recording [%u] index [%u, %u] event count [%u]\n", packet_id, x, y, event_count);
      image_ptr_[data_index] += (uint16_t)event_count;
//      printf("image_ptr_[%u] = %u\n", data_index, image_ptr_[data_index]);
      // Record the packet number
      packet_ids_[packet_id] = 1;
    }

    bool LATRDImageJob::verify_image()
    {
        bool verified = true;

        // First check to see if we have received an EOI packet
        if (eoi_packet_id_ == -1){
            // We have not, so we are not verified
            verified = false;
        }
        if (verified){
          // Loop over the packet IDs and check there are no gaps
          for (uint32_t index = 0; index <= eoi_packet_id_; index++){
            if (packet_ids_.count(index) == 0){
              verified = false;
              break;
            }
          }
        }
      return verified;
    }

    boost::shared_ptr<Frame> LATRDImageJob::to_frame(uint32_t frame_no)
    {
      // Create the frame object to wrap the image
      boost::shared_ptr<Frame> out_frame = boost::shared_ptr<Frame>(new Frame("image"));
      out_frame->copy_data(image_ptr_, width_ * height_ * sizeof(uint16_t));
      out_frame->set_frame_number(frame_no);
      std::vector<dimsize_t> dims(0);
      dims.push_back(height_);
      dims.push_back(width_);
      out_frame->set_dataset_name("image");
      out_frame->set_data_type(1);
      out_frame->set_dimensions(dims);
      return out_frame;
    }

    void LATRDImageJob::reset()
    {
      // We need to reset the memory block
      memset(image_ptr_, 0, (width_ * height_ * sizeof(uint16_t)));
      // Reset the largest_packet_id
      eoi_packet_id_ = -1;
      // Reset the packet id map
      packet_ids_.clear();
      // Reset the sent flag
      sent_ = false;
    }

    void LATRDImageJob::mark_sent()
    {
        sent_ = true;
    }

    bool LATRDImageJob::get_sent()
    {
        return sent_;
    }

} /* namespace FrameProcessor */

