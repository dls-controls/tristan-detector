/*
 * LATRDProcessJob.cpp
 *
 *  Created on: 31 Aug 2017
 *      Author: gnx91527
 */

#include "LATRDProcessJob.h"

namespace FrameProcessor {

LATRDProcessJob::LATRDProcessJob(size_t size) :
		time_slice(0),
		data_ptr(0),
		job_id(0),
		packet_number(0),
		valid_control_words(0),
		timestamp_mismatches(0),
		words_to_process(0),
		valid_results(0)
{
	// Allocate all memory required for stores
	event_ts_ptr = (uint64_t *)malloc(size * sizeof(uint64_t));
	event_id_ptr = (uint32_t *)malloc(size * sizeof(uint32_t));
	event_energy_ptr = (uint32_t *)malloc(size * sizeof(uint32_t));
	ctrl_word_ptr = (uint64_t *)malloc(size * sizeof(uint64_t));
	ctrl_index_ptr = (uint32_t *)malloc(size * sizeof(uint32_t));
}

LATRDProcessJob::~LATRDProcessJob()
{
	// Free all of the previously allocated memory
	if (event_ts_ptr){
		free(event_ts_ptr);
	}
	if (event_id_ptr){
		free(event_id_ptr);
	}
	if (event_energy_ptr){
		free(event_energy_ptr);
	}
	if (ctrl_word_ptr){
		free(ctrl_word_ptr);
	}
	if (ctrl_index_ptr){
		free(ctrl_index_ptr);
	}
}

void LATRDProcessJob::reset()
{
	time_slice = 0;
	data_ptr = 0;
    job_id = 0;
    packet_number = 0;
	valid_control_words = 0;
	timestamp_mismatches = 0;
	words_to_process = 0;
	valid_results = 0;
}

} /* namespace FrameProcessor */
