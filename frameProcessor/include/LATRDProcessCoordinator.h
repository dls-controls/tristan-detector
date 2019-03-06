//
// Created by gnx91527 on 19/07/18.
//

#ifndef LATRD_LATRDPROCESSCOORDINATOR_H
#define LATRD_LATRDPROCESSCOORDINATOR_H

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
#include <stack>

#include "Frame.h"
#include "MetaMessagePublisher.h"
#include "WorkQueue.h"
#include "LATRDBuffer.h"
#include "LATRDDefinitions.h"
#include "LATRDProcessJob.h"
#include "LATRDTimeSliceWrap.h"
#include "LATRDTimestampManager.h"

namespace FrameProcessor {

  class LATRDProcessCoordinator {
  public:
    LATRDProcessCoordinator();

    virtual ~LATRDProcessCoordinator();

    void register_meta_message_publisher(MetaMessagePublisher *ptr);

    void set_acquisition_id(const std::string& acq_id);

    void get_statistics(uint32_t *dropped_packets,
                        uint32_t *invalid_packets,
                        uint32_t *processed_jobs,
                        uint32_t *job_q_size,
                        uint32_t *result_q_size,
                        uint32_t *processed_frames,
                        uint32_t *output_frames);

    void reset_statistics();

    void configure_process(size_t processes, size_t rank);

    std::vector<boost::shared_ptr<Frame> > process_frame(boost::shared_ptr<Frame> frame);

    std::vector<boost::shared_ptr<Frame> > add_jobs_to_buffer(std::vector<boost::shared_ptr<LATRDProcessJob> > jobs);

    std::vector<boost::shared_ptr<Frame> > purge_remaining_buffers();

    void frame_to_jobs(boost::shared_ptr<Frame> frame);

    std::vector<boost::shared_ptr<LATRDProcessJob> > check_for_data_to_write();

    void update_time_slice_meta_data(uint32_t wrap, std::vector<uint32_t> event_counts);

    void purge_time_slice_meta_data();

    void publish_time_slice_meta_data(const std::string& acq_id, uint32_t qty_of_ts);

    std::vector<boost::shared_ptr<LATRDProcessJob> > purge_remaining_jobs();

    void processTask();

    boost::shared_ptr<LATRDProcessJob> getJob();

    void releaseJob(boost::shared_ptr<LATRDProcessJob> job);

    bool processDataWord(uint64_t data_word,
                         uint64_t *previous_course_timestamp,
                         uint64_t *current_course_timestamp,
                         uint32_t packet_number,
                         uint32_t time_slice_wrap,
                         uint32_t time_slice_buffer,
                         uint64_t *event_ts,
                         uint32_t *event_id,
                         uint32_t *event_energy);

    uint64_t getCourseTimestamp(uint64_t data_word);

    uint64_t getFineTimestamp(uint64_t data_word);

    uint16_t getEnergy(uint64_t data_word);

    uint32_t getPositionID(uint64_t data_word);

    uint64_t getFullTimestmap(uint64_t data_word, uint64_t prev_course, uint64_t course);

    uint8_t findTimestampMatch(uint64_t time_stamp);


  private:
    /** Pointer to logger */
    LoggerPtr logger_;

    /** Rank of this process coordinator */
    size_t rank_;

    /** Pointer to worker queue thread */
    boost::thread *thread_[LATRD::number_of_processing_threads];

    /** Pointers to job queues for processing packets and results notification */
    boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > jobQueue_;
    boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > resultsQueue_;

    /** Stack of processing job objects **/
    std::stack<boost::shared_ptr<LATRDProcessJob> > jobStack_;

    /** Current time slice wrap number */
    uint32_t current_ts_wrap_;

    /** Current time slice buffer number */
    uint32_t current_ts_buffer_;

    /** Map to store active time slice buffers */
    std::map<uint32_t, boost::shared_ptr<LATRDTimeSliceWrap> > ts_store_;

    /** Object to record extended timestamps and caculate the deltas */
    LATRDTimestampManager ts_manager_;

      /** Pointer to LATRD buffer and frame manager */
    boost::shared_ptr<LATRDBuffer> timeStampBuffer_;
    boost::shared_ptr<LATRDBuffer> idBuffer_;
    boost::shared_ptr<LATRDBuffer> energyBuffer_;
    boost::shared_ptr<LATRDBuffer> ctrlWordBuffer_;
    boost::shared_ptr<LATRDBuffer> ctrlTimeStampBuffer_;
    uint64_t headerWord1;
    uint64_t headerWord2;

    /** Time slice information used for VDS reconstruction */
    uint32_t last_written_ts_index_;
    std::vector<uint32_t> ts_index_array_;

    /** Meta data publisher */
    MetaMessagePublisher *metaPtr_;

    /** Status information */
    uint32_t dropped_packets_;
    uint32_t invalid_packets_;
    uint32_t processed_jobs_;
    uint32_t processed_frames_;
    uint32_t output_frames_;

    /** Acquisition ID */
    std::string acq_id_;
  };


}

#endif //LATRD_LATRDPROCESSCOORDINATOR_H
