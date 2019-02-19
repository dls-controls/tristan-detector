//
// Created by gnx91527 on 17/09/18.
//

#ifndef LATRD_LATRDTIMESLICEBUFFER_H
#define LATRD_LATRDTIMESLICEBUFFER_H

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
#include "LATRDProcessJob.h"

namespace FrameProcessor {

  class LATRDTimeSliceBuffer
  {
  public:
    LATRDTimeSliceBuffer();
    virtual ~LATRDTimeSliceBuffer();
    void add_job(boost::shared_ptr<LATRDProcessJob> job);
    std::vector<boost::shared_ptr<LATRDProcessJob> > empty();
    size_t size();
    uint32_t no_of_events();
    std::string report();

  private:
    /** Pointer to logger */
    LoggerPtr logger_;

    std::map<uint32_t, boost::shared_ptr<LATRDProcessJob> > job_store_;

    uint32_t no_of_events_;
  };


}

#endif //LATRD_LATRDTIMESLICEBUFFER_H
