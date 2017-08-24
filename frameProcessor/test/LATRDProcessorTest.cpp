/*
 * FileWriterTest.cpp
 *
 */
#define BOOST_TEST_MODULE "FrameProcessorUnitTests"
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <log4cxx/logger.h>
#include <log4cxx/xml/domconfigurator.h>
#include <log4cxx/simplelayout.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
using namespace log4cxx;
using namespace log4cxx::xml;

#include "LATRDBuffer.h"

class GlobalConfig {
public:
  GlobalConfig() {
    //std::cout << "GlobalConfig constructor" << std::endl;
    // Create a default simple console appender for log4cxx.
    consoleAppender = new ConsoleAppender(LayoutPtr(new SimpleLayout()));
    BasicConfigurator::configure(AppenderPtr(consoleAppender));
    Logger::getRootLogger()->setLevel(Level::getWarn());
  }
  ~GlobalConfig() {
    //std::cout << "GlobalConfig constructor" << std::endl;
    //delete consoleAppender;
  };
private:
  ConsoleAppender *consoleAppender;
};
BOOST_GLOBAL_FIXTURE(GlobalConfig);

BOOST_AUTO_TEST_SUITE(BufferUnitTest);

BOOST_AUTO_TEST_CASE(BufferTest)
{
  // Create a buffer with UINT32 type
  FrameProcessor::LATRDBuffer buffer(20, "test_buffer", FrameProcessor::UINT32_TYPE);
  uint64_t data1[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
  boost::shared_ptr<FrameProcessor::Frame> frame;
  // Append 10 points, verify no frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(!frame);
  // Append another 10 points, verify a frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(frame);
  // Verify the frame has frame number 0
  BOOST_CHECK_EQUAL(frame->get_frame_number(), 0);
  // Append 10 points, verify no frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(!frame);
  // Append another 10 points, verify a frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(frame);
  // Verify the frame has frame number 1
  BOOST_CHECK_EQUAL(frame->get_frame_number(), 1);

  // Set the rank and process number
  buffer.configureProcess(3, 1);
  // Append 10 points, verify no frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(!frame);
  // Append another 10 points, verify a frame is returned
  BOOST_CHECK_NO_THROW(frame = buffer.appendData(data1, 10));
  BOOST_CHECK(frame);
  // Verify the frame has frame number 7
  BOOST_CHECK_EQUAL(frame->get_frame_number(), 7);

}


BOOST_AUTO_TEST_SUITE_END(); //BufferUnitTest

