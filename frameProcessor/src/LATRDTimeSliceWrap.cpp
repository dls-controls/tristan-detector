//
// Created by gnx91527 on 18/09/18.
//

#include "LATRDTimeSliceWrap.h"

namespace FrameProcessor {

    LATRDTimeSliceWrap::LATRDTimeSliceWrap(uint32_t no_of_buffers) {
        // Setup logging for the class
        logger_ = Logger::getLogger("FP.LATRDTimeSliceWrap");
        logger_->setLevel(Level::getAll());
        LOG4CXX_TRACE(logger_, "LATRDTimeSliceWrap constructor.");

        // Initialise the time slice buffers for this wrap
        number_of_buffers_ = no_of_buffers;
        for (uint32_t index = 0; index < number_of_buffers_; index++) {
            boost::shared_ptr<LATRDTimeSliceBuffer> ptr = boost::shared_ptr<LATRDTimeSliceBuffer>(
                    new LATRDTimeSliceBuffer());
            buffer_store_[index] = ptr;
        }
    }

    LATRDTimeSliceWrap::~LATRDTimeSliceWrap() {
        buffer_store_.clear();
    }

    void LATRDTimeSliceWrap::add_job(uint32_t buffer_no, boost::shared_ptr<LATRDProcessJob> job) {
        // Check the buffer number supplied is valid
        if (buffer_no < number_of_buffers_) {
            // Add the job into the specified buffer
            buffer_store_[buffer_no]->add_job(job);
        } else {
            // We cannot store this job, which is a serious error
            // TODO: Throw the error
            LOG4CXX_ERROR(logger_, "Job with invalid buffer number: " << buffer_no);
        }
    }

    std::vector<boost::shared_ptr<LATRDProcessJob> > LATRDTimeSliceWrap::empty_all_buffers() {
        std::vector<boost::shared_ptr<LATRDProcessJob> > jobs;
        for (uint32_t index = 0; index < number_of_buffers_; index++) {
            std::vector<boost::shared_ptr<LATRDProcessJob> > buffer_jobs = empty_buffer(index);
            jobs.insert(jobs.end(), buffer_jobs.begin(), buffer_jobs.end());
        }
        return jobs;
    }

    std::vector<boost::shared_ptr<LATRDProcessJob> > LATRDTimeSliceWrap::empty_buffer(uint32_t buffer_number) {
        // Return the processed jobs from the buffer
        return buffer_store_[buffer_number]->empty();
    }

    std::string LATRDTimeSliceWrap::report() {
        // Print a full report of wrap object
        std::stringstream ss;
        ss << "\n==============================================\n"
           << "***              Wrap Report              ***\n"
           << "*** Number of buffers: " << number_of_buffers_ << "\n";
        ss << "==============================================\n";
        for (uint32_t index = 0; index < number_of_buffers_; index++) {
            ss << "*** Buffer " << index;
            ss << buffer_store_[index]->report();
            ss << "\n----------------------------------------------\n";
        }
        ss << "==============================================";
        return ss.str();
    }

}