//
// Created by gnx91527 on 17/09/18.
//

#ifndef LATRD_LATRDTIMESLICEWRAP_H
#define LATRD_LATRDTIMESLICEWRAP_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <boost/shared_ptr.hpp>

#include <stdlib.h>
#include <stdint.h>
#include <map>

#include "LATRDExceptions.h"
#include "LATRDTimeSliceBuffer.h"

namespace FrameProcessor {

  class LATRDTimeSliceWrap
  {
  public:
    LATRDTimeSliceWrap(uint32_t no_of_buffers);
    virtual ~LATRDTimeSliceWrap();
    void add_job(uint32_t buffer_no, boost::shared_ptr<LATRDProcessJob> job);
    std::vector<boost::shared_ptr<LATRDProcessJob> > empty_all_buffers();
    std::vector<boost::shared_ptr<LATRDProcessJob> > empty_buffer(uint32_t buffer_number);
    std::string report();

  private:
    /** Pointer to logger */
    LoggerPtr logger_;
    uint32_t number_of_buffers_;

    std::map<uint32_t, boost::shared_ptr<LATRDTimeSliceBuffer> > buffer_store_;
  };


}

#endif //LATRD_LATRDTIMESLICEWRAP_H
