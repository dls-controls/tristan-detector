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
#include <DebugLevelLogger.h>

#include "LATRDFrameDecoder.h"

IMPLEMENT_DEBUG_LEVEL

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

    // Verify the packet number is 1
    uint32_t packet_number = decoder->get_packet_number();
    BOOST_CHECK_EQUAL(packet_number, 1);

    // Verify the producer ID is 1
    uint32_t producer_ID = decoder->get_producer_ID();
    BOOST_CHECK_EQUAL(producer_ID, 1);

    // Verify the word count is 1024
    uint32_t word_count = decoder->get_word_count();
    BOOST_CHECK_EQUAL(word_count, 1024);

    // Verify the time slice is 1
    uint32_t time_slice = decoder->get_time_slice();
    BOOST_CHECK_EQUAL(time_slice, 1);


    // Write a sensible header value
    // Producer ID - 8
    // Time Slice  - 10241024
    // Word Count  - 1022
    // Packet Number - 512256
    hdrPtr[0] = 0xE0200271100003FE;
    hdrPtr[1] = 0xE00000000007D100;

    // Call decode header on the decoder
    decoder->process_packet_header(16, 9999, &from_addr);

    // Verify the packet number is 1
    packet_number = decoder->get_packet_number();
    BOOST_CHECK_EQUAL(packet_number, 512256);

    // Verify the producer ID is 8
    producer_ID = decoder->get_producer_ID();
    BOOST_CHECK_EQUAL(producer_ID, 8);

    // Verify the word count is 1022
    word_count = decoder->get_word_count();
    BOOST_CHECK_EQUAL(word_count, 1022);

    // Verify the time slice is 1
    time_slice = decoder->get_time_slice();
    BOOST_CHECK_EQUAL(time_slice, 10241024);

}

BOOST_AUTO_TEST_SUITE_END();

