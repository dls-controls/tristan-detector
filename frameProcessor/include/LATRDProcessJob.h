/*
 * LATRDProcessJob.h
 *
 *  Created on: 31 Aug 2017
 *      Author: gnx91527
 */

#ifndef FRAMEPROCESSOR_SRC_LATRDPROCESSJOB_H_
#define FRAMEPROCESSOR_SRC_LATRDPROCESSJOB_H_

#include <stdlib.h>
#include <stdint.h>

namespace FrameProcessor {

class LATRDProcessJob
{
public:
	LATRDProcessJob(size_t size);
	virtual ~LATRDProcessJob();
	void reset();

	uint32_t job_id;
	uint16_t words_to_process;
	uint32_t time_slice;
	uint16_t valid_results;
	uint16_t valid_control_words;
	uint16_t timestamp_mismatches;
	uint64_t *data_ptr;
	uint64_t *event_ts_ptr;
	uint32_t *event_id_ptr;
	uint32_t *event_energy_ptr;
	uint64_t *ctrl_word_ptr;
	uint32_t *ctrl_index_ptr;
};

} /* namespace FrameProcessor */

#endif /* FRAMEPROCESSOR_SRC_LATRDPROCESSJOB_H_ */
