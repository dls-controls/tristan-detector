//
// Created by gnx91527 on 31/07/18.
//

#ifndef LATRD_LATRDTIMESTAMPMANAGER_H
#define LATRD_LATRDTIMESTAMPMANAGER_H

#include <log4cxx/logger.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/helpers/exception.h>

#include <boost/thread.hpp>

using namespace log4cxx;
using namespace log4cxx::helpers;

#include <stdlib.h>
#include <stdint.h>
#include <map>

namespace FrameProcessor {
    //! LATRDTimestampException - custom exception class implementing "what" for error string
    class LATRDTimestampException : public std::exception {
    public:

        //! Create LATRDTimestampException with no message
        LATRDTimestampException(void) throw() :
                what_("") {};

        //! Creates LATRDTimestampException with informational message
        LATRDTimestampException(const std::string what) throw() :
                what_(what) {};

        //! Returns the content of the informational message
        virtual const char *what(void) const throw() {
            return what_.c_str();
        };

        //! Destructor
        ~LATRDTimestampException(void) throw() {};

    private:

        // Member variables
        const std::string what_;  //!< Informational message about the exception

    }; // LATRDTimestampException

    class LATRDTimestampManager {
    public:
        LATRDTimestampManager();

        virtual ~LATRDTimestampManager();

        void clear();

        void reset_delta();

        void add_timestamp(uint32_t packet_number, uint64_t timestamp);

        uint64_t read_delta();

    private:
        /** Pointer to logger */
        LoggerPtr logger_;

        /** Mutex used to make this class thread safe */
        boost::recursive_mutex mutex_;

        std::map <uint32_t, uint64_t> timestamps_;
        uint64_t delta_timestamp_;
    };


}

#endif //LATRD_LATRDTIMESTAMPMANAGER_H
