//
// Created by gnx91527 on 31/07/18.
//

#include "LATRDTimestampManager.h"

namespace FrameProcessor {

    LATRDTimestampManager::LATRDTimestampManager() :
            delta_timestamp_(0),
            time_slice_buffer_(0),
            time_slice_wrap_(0)
    {
        // Setup logging for the class
        logger_ = Logger::getLogger("FP.LATRDTimestampManager");
        LOG4CXX_TRACE(logger_, "LATRDTimestampManager constructor.");
    }

    LATRDTimestampManager::~LATRDTimestampManager() {

    }

    void LATRDTimestampManager::clear() {
        // Protect this method
        boost::lock_guard<boost::recursive_mutex> lock(mutex_);
        timestamps_.clear();
    }

    void LATRDTimestampManager::reset_delta() {
        // Protect this method
        boost::lock_guard<boost::recursive_mutex> lock(mutex_);
        delta_timestamp_ = 0;
    }

    void LATRDTimestampManager::add_timestamp(uint32_t time_slice_buffer,
                                              uint32_t time_slice_wrap,
                                              uint32_t packet_number,
                                              uint64_t timestamp) {
        // Protect this method
        boost::lock_guard<boost::recursive_mutex> lock(mutex_);
        LOG4CXX_ERROR(logger_,
                      "add_timestamp called with packet number [" << packet_number << "] timestamp [" << timestamp
                                                                  << "]");
        // Check if this is the first timestamp
        if (timestamps_.empty()) {
            // Record the time slice wrap and buffer number
            time_slice_buffer_ = time_slice_buffer;
            time_slice_wrap_ = time_slice_wrap;
        } else {
            // We can only use packets from the same time slice buffer and wrap
            if (time_slice_buffer == time_slice_buffer_ && time_slice_wrap == time_slice_wrap_) {
                // Check if we have a timestamp for this packet already
                if (timestamps_.count(packet_number) > 0) {
                    // If we do then calculate the difference, check delta t is not different
                    uint64_t delta = timestamp - timestamps_[packet_number];
                    // We only need to check if the delta is non zero (a zero delta means we have had the same extended timestamp twice)
                    if (delta > 0) {
                        if (delta_timestamp_ > 0) {
                            if (delta != delta_timestamp_) {
                                // This is potentially a serious error so log it and raise an exception
                                LOG4CXX_ERROR(logger_, "Delta extended timestamp values are not consistent. Old delta ["
                                    << delta_timestamp_ << "] new delta [" << delta << "]");
                                LOG4CXX_ERROR(logger_, "Packet number [" << packet_number << "]");
                                LOG4CXX_ERROR(logger_,
                                              "Timestamp [" << timestamp << "] older timestamp ["
                                                            << timestamps_[packet_number]
                                                            << "]");
                                throw LATRDTimestampException("Delta extended timestamp values are not consistent");
                            }
                        } else {
                            // Record the delta timestamp
                            delta_timestamp_ = delta;
                            LOG4CXX_ERROR(logger_, "New delta [" << delta_timestamp_ << "]");
                        }
                        // Save the timestamp data
                        timestamps_[packet_number] = timestamp;
                    }
                } else {
                    // Save the timestamp data
                    timestamps_[packet_number] = timestamp;
                    // Step back through consecutive packets looking for a different timestamp
                    // Start at a packet number of 1 less than the current
                    // Walk backwards until either we hit packet number 1, or a gap or a different timestamp
                    uint32_t index = packet_number;
                    bool backtracking = true;
                    while (backtracking) {
                        index -= 1;
                        if (timestamps_.count(index) > 0) {
                            // If we do then calculate the difference, check delta t is not different
                            uint64_t delta = timestamp - timestamps_[index];
                            // We only need to check if the delta is non zero (a zero delta means we have had the same extended timestamp twice)
                            if (delta > 0) {
                                if (delta_timestamp_ > 0) {
                                    if (delta != delta_timestamp_) {
                                        // This is potentially a serious error so log it and raise an exception
                                        LOG4CXX_ERROR(logger_,
                                                      "Delta extended timestamp values are not consistent. Old delta ["
                                                          << delta_timestamp_ << "] new delta [" << delta << "]");
                                        LOG4CXX_ERROR(logger_,
                                                      "Packet number [" << packet_number << "] index [" << index
                                                                        << "]");
                                        LOG4CXX_ERROR(logger_, "Timestamp [" << timestamp << "] older timestamp ["
                                                                             << timestamps_[index] << "]");
                                        throw LATRDTimestampException(
                                            "Delta extended timestamp values are not consistent");
                                    } else {
                                        // We have verification that the timestamp delta is good so drop out of the backtrack loop
                                        backtracking = false;
                                    }
                                } else {
                                    // Record the delta timestamp
                                    delta_timestamp_ = delta;
                                    LOG4CXX_ERROR(logger_,
                                                  "Packet number [" << packet_number << "] index [" << index << "]");
                                    LOG4CXX_ERROR(logger_,
                                                  "New delta whilst backtracking [" << delta_timestamp_ << "]");
                                    // Drop out of the backtrack loop
                                    backtracking = false;
                                }
                            } else {
                                backtracking = false;
                            }
                        } else {
                            // There is a gap in the packets so we cannot verify this timestamp against a previous
                            // Drop out of the backtrack loop
                            backtracking = false;
                        }
                    }
                }
            }
        }
    }

    uint64_t LATRDTimestampManager::read_delta() {
        // Protect this method
        boost::lock_guard<boost::recursive_mutex> lock(mutex_);
        return delta_timestamp_;
    }

}