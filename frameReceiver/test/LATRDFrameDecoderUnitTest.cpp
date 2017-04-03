/*
 * LATRDFrameDecoderUnitTest.cpp
 *
 *  Created on: 3 Apr 2017
 *      Author: gnx91527
 */
#define BOOST_TEST_MODULE 1

#include <boost/test/unit_test.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <log4cxx/logger.h>
#include <log4cxx/consoleappender.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/simplelayout.h>

#include "LATRDFrameDecoder.h"

class FrameDecoderTestFixture
{
public:
    FrameDecoderTestFixture() :
        logger(log4cxx::Logger::getLogger("LATRDFrameDecoderUnitTest"))
    {

    }
    log4cxx::LoggerPtr logger;
};

BOOST_FIXTURE_TEST_SUITE(LATRDFrameDecoderUnitTest, FrameDecoderTestFixture);

BOOST_AUTO_TEST_CASE( LATRDDecoderLibraryTest )
{
    boost::shared_ptr<FrameReceiver::FrameDecoder> decoder(new FrameReceiver::LATRDFrameDecoder());
    BOOST_CHECK_NO_THROW(decoder->init(logger));

    // Verify the size of a LATRD buffer is 10*1024+40.
    // That is 10 packets of 1024 bytes plus a header of 40 bytes
    size_t buffer_size = decoder->get_frame_buffer_size();
    BOOST_CHECK_EQUAL(buffer_size, 81960);

    // Verify the header size of a LATRD buffer is 40 bytes
    size_t header_size = decoder->get_frame_header_size();
    BOOST_CHECK_EQUAL(header_size, 40);

    // Verify the packet header buffer is 16 bytes (2*64bit values)
    size_t pkt_header_size = decoder->get_packet_header_size();
    BOOST_CHECK_EQUAL(pkt_header_size, 16);

    // Verify that the decoding of LATRD packets requires a header peek
    bool require_header_peek = decoder->requires_header_peek();
    BOOST_CHECK_EQUAL(require_header_peek, true);

    // Get a pointer to the header buffer
    unsigned char *hdrPtr = (unsigned char *)decoder->get_packet_header_buffer();

    // Write a sensible header value
    // Call decode header on the decoder
    // Verify the frame number
    // Verify the packet number
    // Verify the number of empty buffers
    // Verify the number of mapped buffers

    decoder->get_packet_header_buffer();

    decoder->get_next_payload_buffer();

    decoder->get_next_payload_size();

    decoder->get_num_empty_buffers();

    decoder->get_num_mapped_buffers();

}

BOOST_AUTO_TEST_SUITE_END();

