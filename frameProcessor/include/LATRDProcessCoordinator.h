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

#include "Frame.h"

namespace FrameProcessor {

    class LATRDProcessCoordinator
    {
    public:
        LATRDProcessCoordinator(size_t ts_buffers);
        virtual ~LATRDProcessCoordinator();
        std::vector<boost::shared_ptr<Frame> > add_frame(uint32_t ts_wrap, uint32_t ts_buffer, boost::shared_ptr<Frame> frame);
        boost::shared_ptr<Frame> get_frame(uint32_t ts_wrap, uint32_t ts_buffer, uint32_t frame_number);
        std::vector<boost::shared_ptr<Frame> > clear_buffer(uint32_t ts_wrap, uint32_t ts_buffer);
        std::vector<boost::shared_ptr<Frame> > clear_all_buffers();


    private:
        /** Pointer to logger */
        LoggerPtr logger_;

        size_t number_of_ts_buffers_;
        std::vector<std::map<uint32_t, boost::shared_ptr<Frame> > > frame_store_;
        std::vector<uint32_t> ts_buffer_wrap_;
    };


}

#endif //LATRD_LATRDPROCESSCOORDINATOR_H
