//
// Created by gnx91527 on 31/07/18.
//

#include "LATRDTimestampManager.h"

namespace FrameProcessor {

  LATRDTimestampManager::LATRDTimestampManager() : delta_timestamp_(0)
  {

  }

  LATRDTimestampManager::~LATRDTimestampManager()
  {

  }

  void LATRDTimestampManager::clear()
  {
    delta_timestamp_ = 0;
    timestamps_.clear();
  }

  void LATRDTimestampManager::add_timestamp(uint32_t packet_number, uint64_t timestamp)
  {
    // Check if we have a timestamp for this packet already
    if (timestamps_.count(packet_number) > 0){
      // If we do then calculate the difference, check delta t is not different
      uint64_t delta = timestamp - timestamps_[packet_number];
      if (delta_timestamp_ > 0){
        if (delta != delta_timestamp_){
          // This is potentially a serious error so log it and raise an exception?
          // TODO: Decide if we need to raise an exception
          // TODO: Log the error
        }
      } else {
        // Record the delta timestamp
        delta_timestamp_ = delta;
      }
    } else {
      // Step back through consecutive packets looking for a different timestamp
      // Start at a packet number of 1 less than the current
      // Walk backwards until either we hit packet number 1, or a gap or a different timestamp
    }

    // Record this timestamp


  }

  uint64_t LATRDTimestampManager::read_delta()
  {
    return delta_timestamp_;
  }

}