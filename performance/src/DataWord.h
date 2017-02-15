/*
 * DataWord.h
 *
 *  Created on: 6 Feb 2017
 *      Author: gnx91527
 */

#ifndef SRC_DATAWORD_H_
#define SRC_DATAWORD_H_

#include <string>
#include <iostream>
#include <sstream>
#include <sys/types.h>

class DataWord {
public:
	DataWord(const std::string& word);
	DataWord(u_int64_t rawWord);
	virtual ~DataWord();
	u_int64_t getRaw();
	bool isCtrl();
	int ctrlType();
	u_int64_t timestampCourse();

private:
	std::string hexWord_;
	u_int64_t rawWord_;
};

#endif /* SRC_DATAWORD_H_ */
