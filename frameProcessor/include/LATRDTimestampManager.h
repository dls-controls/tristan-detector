//
// Created by gnx91527 on 31/07/18.
//

#ifndef LATRD_LATRDTIMESTAMPMANAGER_H
#define LATRD_LATRDTIMESTAMPMANAGER_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <stdlib.h>
#include <stdint.h>
#include <map>

namespace FrameProcessor {

class LATRDTimestampManager
{
  public:
    LATRDTimestampManager();
    virtual ~LATRDTimestampManager();
    void clear();
    void add_timestamp(uint32_t packet_number, uint64_t timestamp);
    uint64_t read_delta();

  private:
    /** Pointer to logger */
    LoggerPtr logger_;

    std::map<uint32_t, uint64_t> timestamps_;
    uint64_t delta_timestamp_;
  };


}

#endif //LATRD_LATRDTIMESTAMPMANAGER_H
