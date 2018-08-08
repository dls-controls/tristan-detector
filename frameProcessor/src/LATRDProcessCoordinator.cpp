//
// Created by gnx91527 on 19/07/18.
//

#include "LATRDProcessCoordinator.h"

namespace FrameProcessor {

    LATRDProcessCoordinator::LATRDProcessCoordinator(size_t ts_buffers)
    {
        // Setup logging for the class
        logger_ = Logger::getLogger("FP.LATRDProcessCoordinator");
        logger_->setLevel(Level::getDebug());
        LOG4CXX_TRACE(logger_, "LATRDProcessCoordinator constructor.");

        number_of_ts_buffers_ = ts_buffers;
        LOG4CXX_DEBUG(logger_, "Setting the number of time slice buffer to " << ts_buffers);

        for (size_t index = 0; index < ts_buffers; index++){
            // Create a vector of maps, one for each time slice buffer
            frame_store_.push_back(std::map<uint32_t, boost::shared_ptr<Frame> >());

            // Initialise the vector to store the current modulo time slice for each buffer
            ts_buffer_wrap_.push_back(0);
        }

    }

    LATRDProcessCoordinator::~LATRDProcessCoordinator()
    {

    }

    std::vector<boost::shared_ptr<Frame> > LATRDProcessCoordinator::add_frame(uint32_t ts_wrap, uint32_t ts_buffer, boost::shared_ptr<Frame> frame)
    {
        LOG4CXX_DEBUG(logger_, "Adding frame " << frame->get_frame_number() << " with ts wrap[" << ts_wrap << "] ts buffer[" << ts_buffer << "]");
        // Vector to store any returned frames
        std::vector<boost::shared_ptr<Frame> > return_frames;
        // Check that for this buffer number the wrap matches our stored value
        if (ts_buffer_wrap_[ts_buffer] == ts_wrap){
            LOG4CXX_DEBUG(logger_, "ts wrap matched");
            // Store the frame in the correct map by its ID
            frame_store_[ts_buffer][frame->get_frame_number()] = frame;
        } else {
            // New wrap value detected
            // If the map contains any remaining elements then pass them back as an array in order
            if (frame_store_[ts_buffer].size() > 0){
                LOG4CXX_DEBUG(logger_, "New ts wrap, returning " << frame_store_[ts_buffer].size() << " frames for processing");
                // First iterate over the map and store the pointers in the return vector
                for(std::map<uint32_t, boost::shared_ptr<Frame> >::iterator it = frame_store_[ts_buffer].begin(); it != frame_store_[ts_buffer].end(); ++it){
                    return_frames.push_back(it->second);
                }
                // Now clear out those vectors
                frame_store_[ts_buffer].clear();
            }
            // Store the new wrap number
            ts_buffer_wrap_[ts_buffer] = ts_wrap;
            // And store the frame
            frame_store_[ts_buffer][frame->get_frame_number()] = frame;
        }
        // Return any frames ready to process
        return return_frames;
    }

    boost::shared_ptr<Frame> LATRDProcessCoordinator::get_frame(uint32_t ts_wrap, uint32_t ts_buffer, uint32_t frame_number)
    {
        // Init the return shared pointer to an empty pointer
        boost::shared_ptr<Frame> return_frame;

        // Check the wrap number is correct for the buffer
        if (ts_buffer_wrap_[ts_buffer] == ts_wrap){
            LOG4CXX_DEBUG(logger_, "Frame " << frame_number << " ts wrap[" << ts_wrap << "] ts buffer[" << ts_buffer << "] found, returning");
            // Check if the frame with the number exists in the store
            if (frame_store_[ts_buffer].count(frame_number) > 0){
                // Remove the frame pointer from the store and return it
                return_frame = frame_store_[ts_buffer][frame_number];
                frame_store_[ts_buffer].erase(frame_number);
            }
        }
        return return_frame;
    }

    std::vector<boost::shared_ptr<Frame> > LATRDProcessCoordinator::clear_buffer(uint32_t ts_wrap, uint32_t ts_buffer)
    {
        // Vector to store any returned frames
        std::vector<boost::shared_ptr<Frame> > return_frames;
        // Check that for this buffer number the wrap matches our stored value
        if (ts_buffer_wrap_[ts_buffer] == ts_wrap){
            // If the map contains any remaining elements then pass them back as an array in order
            if (frame_store_[ts_buffer].size() > 0){
                LOG4CXX_DEBUG(logger_, "Clearing out " << frame_store_[ts_buffer].size() << " frames with ts wrap[" << ts_wrap << "] ts buffer[" << ts_buffer << "]");
                // First iterate over the map and store the pointers in the return vector
                for(std::map<uint32_t, boost::shared_ptr<Frame> >::iterator it = frame_store_[ts_buffer].begin(); it != frame_store_[ts_buffer].end(); ++it){
                    return_frames.push_back(it->second);
                }
                // Now clear out those vectors
                frame_store_[ts_buffer].clear();
            }
        }
        // Return any frames ready to process
        return return_frames;
    }

    std::vector<boost::shared_ptr<Frame> > LATRDProcessCoordinator::clear_all_buffers()
    {
        // Vector to store any returned frames
        std::vector<boost::shared_ptr<Frame> > return_frames;

        // Find the first buffer to work on
        int buff_num = -1;
        int buff_index = 0;
        for (size_t index = 0; index < number_of_ts_buffers_; index++){
            if (buff_num == -1){
                buff_num = ts_buffer_wrap_[index];
                buff_index = index;
            } else if (ts_buffer_wrap_[index] < buff_num){
                buff_num = ts_buffer_wrap_[index];
                buff_index = index;
            }
        }
        // Now empty each in order
        for (size_t index = 0; index < number_of_ts_buffers_; index++){
            if (frame_store_[buff_index].size() > 0) {
                // Iterate over the map and store the pointers in the return vector
                for (std::map < uint32_t, boost::shared_ptr < Frame > > ::iterator it = frame_store_[buff_index].begin(); it != frame_store_[buff_index].end(); ++it){
                    return_frames.push_back(it->second);
                }
                // Now clear out those vectors
                frame_store_[buff_index].clear();
            }
            buff_index++;
            if (buff_index >= number_of_ts_buffers_){
                buff_index = 0;
            }
        }
        return return_frames;
    }

}