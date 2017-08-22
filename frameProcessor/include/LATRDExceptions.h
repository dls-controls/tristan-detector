/*
 * LATRDExceptions.h
 *
 *  Created on: 17 Aug 2017
 *      Author: gnx91527
 */

#ifndef FRAMEPROCESSOR_INCLUDE_LATRDEXCEPTIONS_H_
#define FRAMEPROCESSOR_INCLUDE_LATRDEXCEPTIONS_H_
#include <exception>

namespace FrameProcessor
{
class LATRDProcessingException: public std::runtime_error
{
public:
	LATRDProcessingException(const std::string& message) :
		msg_(message),
		std::runtime_error(message)
	{

	}

	virtual ~LATRDProcessingException() throw()
	{
	}

	virtual const char* what() const throw()
	{
		return msg_.c_str();
	}

private:
	std::string msg_;
};

class LATRDTimestampMismatchException: public LATRDProcessingException
{
public:
	LATRDTimestampMismatchException(const std::string& message) :
		LATRDProcessingException(message)
	{
	}

	virtual ~LATRDTimestampMismatchException() throw()
	{
	}
};

}

#endif /* FRAMEPROCESSOR_INCLUDE_LATRDEXCEPTIONS_H_ */
