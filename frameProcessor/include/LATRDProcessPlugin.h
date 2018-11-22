/*
 * LATRDProcessPlugin.h
 *
 *  Created on: 26 Apr 2017
 *      Author: gnx91527
 */

#ifndef FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_
#define FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <stack>
#include "FrameProcessorPlugin.h"
#include "WorkQueue.h"
#include "LATRDDefinitions.h"
#include "LATRDExceptions.h"
#include "LATRDBuffer.h"
#include "LATRDProcessJob.h"
#include "LATRDProcessCoordinator.h"
#include "LATRDProcessIntegral.h"
#include "LATRDTimestampManager.h"
#include "ClassLoader.h"

namespace FrameProcessor {
/*    enum LATRDDataControlType {
        Unknown, HeaderWord0, HeaderWord1, ExtendedTimestamp, IdleControlWord
    };*/

    class LATRDProcessPlugin : public FrameProcessorPlugin {
    public:
        LATRDProcessPlugin();

        virtual ~LATRDProcessPlugin();

        void configure(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

        void requestConfiguration(OdinData::IpcMessage &reply);

        void status(OdinData::IpcMessage& status);

        void configureProcess(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

        void configureSensor(OdinData::IpcMessage &config, OdinData::IpcMessage &reply);

        void createMetaHeader();

        bool reset_statistics(void);

    int get_version_major();
    int get_version_minor();
    int get_version_patch();
    std::string get_version_short();
    std::string get_version_long();

    private:

        /** Constant for this decoders name when publishing meta data */
        static const std::string META_NAME;

        /** Configuration constant for setting operational mode */
        static const std::string CONFIG_MODE;
        static const std::string CONFIG_MODE_TIME_ENERGY;
        static const std::string CONFIG_MODE_TIME_ONLY;
        static const std::string CONFIG_MODE_COUNT;

        /** Configuration constant for process related items */
        static const std::string CONFIG_SENSOR;
        static const std::string CONFIG_SENSOR_WIDTH;
        static const std::string CONFIG_SENSOR_HEIGHT;

      /** Configuration constant for setting raw mode */
        static const std::string CONFIG_RAW_MODE;

        /** Configuration constant for resetting the frame counter */
        static const std::string CONFIG_RESET_FRAME;

        /** Configuration constant for process related items */
        static const std::string CONFIG_PROCESS;
        /** Configuration constant for number of processes */
        static const std::string CONFIG_PROCESS_NUMBER;
        /** Configuration constant for this process rank */
        static const std::string CONFIG_PROCESS_RANK;

        /** Pointer to logger */
        LoggerPtr logger_;
        /** Mutex used to make this class thread safe */
        boost::recursive_mutex mutex_;

        /** Frame coordinator object */
        LATRDProcessCoordinator coordinator_;

        /** Integral mode processing object */
        LATRDProcessIntegral integral_;

        /** Timestamp manager object */
//        LATRDTimestampManager ts_manager_;

        /** Pointers to job queues for processing packets and results notification */
//        boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > jobQueue_;
//        boost::shared_ptr<WorkQueue<boost::shared_ptr<LATRDProcessJob> > > resultsQueue_;

        /** Stack of processing job objects **/
//        std::stack<boost::shared_ptr<LATRDProcessJob> > jobStack_;

        /** Map containing extended timestamps for the current time slice **/
//        std::map<uint32_t, uint64_t> extended_timestamps_;

        /** Pointer to LATRD buffer and frame manager */
        boost::shared_ptr<LATRDBuffer> rawBuffer_;
//        boost::shared_ptr<LATRDBuffer> timeStampBuffer_;
//        boost::shared_ptr<LATRDBuffer> idBuffer_;
//        boost::shared_ptr<LATRDBuffer> energyBuffer_;

        rapidjson::Document meta_document_;

        size_t sensor_width_;
        size_t sensor_height_;

        std::string mode_;

        size_t concurrent_processes_;
        size_t concurrent_rank_;

        /** Last processed information */
//        uint32_t last_processed_ts_wrap_;
//        uint32_t last_processed_ts_buffer_;
//        uint32_t last_processed_frame_number_;
//        uint32_t last_processed_was_idle_;

        /** Management of time slice information **/
        uint64_t current_point_index_;
        uint32_t current_time_slice_;

        /** Process raw mode **/
        uint32_t raw_mode_;

//        boost::shared_ptr<LATRDProcessJob> getJob();

//        void releaseJob(boost::shared_ptr<LATRDProcessJob> job);

        void dump_frame(boost::shared_ptr<Frame> frame);

        void process_frame(boost::shared_ptr<Frame> frame);

        void process_raw(boost::shared_ptr<Frame> frame);

        void publishControlMetaData(boost::shared_ptr<LATRDProcessJob> job);

    };

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_INCLUDE_LATRDPROCESSPLUGIN_H_ */
