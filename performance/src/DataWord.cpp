/*
 * DataWord.cpp
 *
 *  Created on: 6 Feb 2017
 *      Author: gnx91527
 */

#include "DataWord.h"

DataWord::DataWord(const std::string& word) :
	rawWord_(0)
{
	hexWord_ = word;
    std::stringstream ss;
    ss << std::hex << word;
    ss >> rawWord_;
}

DataWord::DataWord(u_int64_t rawWord) :
	rawWord_(rawWord)
{
}

DataWord::~DataWord()
{
	// TODO Auto-generated destructor stub
}

u_int64_t DataWord::getRaw()
{
	return rawWord_;
}

bool DataWord::isCtrl()
{
	bool ctrl = false;
    if (rawWord_ & 0x8000000000000000){
    	ctrl = true;
    }
    return ctrl;
}

int DataWord::ctrlType()
{
	int ctrlType = 0;
	if (this->isCtrl()){
		ctrlType = (rawWord_ >> 58) & 0x03F;
	} else {
		ctrlType = -1;
	}
	return ctrlType;
}

u_int64_t DataWord::timestampCourse()
{
	u_int64_t tsCourse = 0;
	if (this->isCtrl()){
		tsCourse = rawWord_ & 0x000FFFFFFFFFFFF8;
	}
	return tsCourse;
}

