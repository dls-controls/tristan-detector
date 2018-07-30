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
#include "LATRDProcessCoordinator.h"

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


BOOST_AUTO_TEST_SUITE(CoordinatorUnitTest);

BOOST_AUTO_TEST_CASE(CoordinatorTest)
{
  // Create a coordinator class with 8 time slice buffers
  FrameProcessor::LATRDProcessCoordinator cd(8);

  // Create some frames with appropriate IDs
  unsigned long long ids[26] = {1, 2, 3, 4, 5,
                                1, 2, 3,
                                1, 2, 3,
                                1, 2, 3,
                                1, 2, 3,
                                1, 2, 3,
                                1, 2, 3,
                                1, 2, 3
  };

  boost::shared_ptr<FrameProcessor::Frame> frames[26];
  for (int index = 0; index < 26; index++){
    frames[index] = boost::shared_ptr<FrameProcessor::Frame>(new FrameProcessor::Frame("test"));
    frames[index]->set_frame_number(ids[index]);
  }

  // Add frames 1, 2, 4, 5 with wrap 0, buffer 0
  cd.add_frame(0, 0, frames[0]);
  cd.add_frame(0, 0, frames[1]);
  cd.add_frame(0, 0, frames[3]);
  cd.add_frame(0, 0, frames[4]);

  // Now request frames 1 to 5, should get back NULL for 3
  boost::shared_ptr<FrameProcessor::Frame> fptr;

  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 1));
  BOOST_CHECK_EQUAL(fptr->get_frame_number(), 1);
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 2));
  BOOST_CHECK_EQUAL(fptr->get_frame_number(), 2);
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 3));
  BOOST_CHECK(!fptr);
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 4));
  BOOST_CHECK_EQUAL(fptr->get_frame_number(), 4);
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 5));
  BOOST_CHECK_EQUAL(fptr->get_frame_number(), 5);

  // Now request frames 1 and 2 again, and check we get back NULL
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 1));
  BOOST_CHECK(!fptr);
  BOOST_CHECK_NO_THROW(fptr = cd.get_frame(0, 0, 2));
  BOOST_CHECK(!fptr);

  // Add frames 1, 2, 3 with wrap 0, buffer 0
  cd.add_frame(0, 0, frames[0]);
  cd.add_frame(0, 0, frames[1]);
  cd.add_frame(0, 0, frames[2]);

  // Add frame 1 with wrap 0, buffer 1 and verify we get an empty array back
  std::vector<boost::shared_ptr<FrameProcessor::Frame> > vec;
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 1, frames[0]));
  BOOST_CHECK_EQUAL(vec.size(), 0);

  // Add frame 1 with wrap 0, buffer 2 and verify we get an empty array back
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 2, frames[0]));
  BOOST_CHECK_EQUAL(vec.size(), 0);

  // Add frame 1 with wrap 0, buffer 3 and verify we get an empty array back
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 3, frames[0]));
  BOOST_CHECK_EQUAL(vec.size(), 0);

  // Add frame 1 with wrap 0, buffer 4 and verify we get an empty array back
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 4, frames[0]));
  BOOST_CHECK_EQUAL(vec.size(), 0);

  // Add frame 1 with wrap 1 buffer 0 and verify that we get back an array of size 3
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(1, 0, frames[0]));
  BOOST_CHECK_EQUAL(vec.size(), 3);
  // Verify that the vector contains frames 1, 2, 3
  BOOST_CHECK_EQUAL(vec[0]->get_frame_number(), 1);
  BOOST_CHECK_EQUAL(vec[1]->get_frame_number(), 2);
  BOOST_CHECK_EQUAL(vec[2]->get_frame_number(), 3);

  // Add frame 2 with wrap 0, buffer 1 and verify we get an empty array back
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 1, frames[1]));
  BOOST_CHECK_EQUAL(vec.size(), 0);
  // Add frame 4 with wrap 0, buffer 1 and verify we get an empty array back
  BOOST_CHECK_NO_THROW(vec = cd.add_frame(0, 1, frames[3]));
  BOOST_CHECK_EQUAL(vec.size(), 0);

  // Request to clear wrap 0, buffer 1 and verify we get back an array of size 3
  BOOST_CHECK_NO_THROW(vec = cd.clear_buffer(0, 1));
  BOOST_CHECK_EQUAL(vec.size(), 3);
  // Verify that the vector contains frames 1, 2, 4
  BOOST_CHECK_EQUAL(vec[0]->get_frame_number(), 1);
  BOOST_CHECK_EQUAL(vec[1]->get_frame_number(), 2);
  BOOST_CHECK_EQUAL(vec[2]->get_frame_number(), 4);

}

BOOST_AUTO_TEST_SUITE_END(); //CoordinatorUnitTest
