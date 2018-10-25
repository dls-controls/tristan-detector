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
    boost::shared_ptr<FrameReceiver::LATRDFrameDecoder> decoder(new FrameReceiver::LATRDFrameDecoder());
    OdinData::IpcMessage config_msg;
    BOOST_CHECK_NO_THROW(decoder->init(logger, config_msg));

    // Verify the size of a LATRD buffer is (100*1024*8)+136.
    // That is 100 packets of 1024x8 bytes plus a header of 136 bytes
    size_t buffer_size = decoder->get_frame_buffer_size();
    BOOST_CHECK_EQUAL(buffer_size, 819336);

    // Verify the header size of a LATRD buffer is 136 bytes
    size_t header_size = decoder->get_frame_header_size();
    BOOST_CHECK_EQUAL(header_size, 136);

    // Verify the packet header buffer is 24 bytes (3*64bit values)
    size_t pkt_header_size = decoder->get_packet_header_size();
    BOOST_CHECK_EQUAL(pkt_header_size, 24);

    // Verify that the decoding of LATRD packets requires a header peek
    bool require_header_peek = decoder->requires_header_peek();
    BOOST_CHECK_EQUAL(require_header_peek, true);

    // Get a pointer to the header buffer
    uint64_t *hdrPtr = (uint64_t *)decoder->get_packet_header_buffer();

    // Write a sensible header value
    // Producer ID - 1
    // Time Slice  - 1
    // Word Count  - 1024
    // Packet Number - 1
    hdrPtr[0] = 0xE004000000040400;
    hdrPtr[1] = 0xE000000000000001;

    // Call decode header on the decoder
	struct sockaddr_in from_addr;
    decoder->process_packet_header(16, 9999, &from_addr);

    // Write a sensible header value
    // Producer ID - 8
    // Time Slice  - 10241024
    // Word Count  - 1022
    // Packet Number - 512256
    hdrPtr[0] = 0xE0200271100003FE;
    hdrPtr[1] = 0xE00000000007D100;

    // Call decode header on the decoder
    decoder->process_packet_header(16, 9999, &from_addr);

}

BOOST_AUTO_TEST_SUITE_END();

