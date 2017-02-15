/*
 * AsciiStackReader.h
 *
 *  Created on: 6 Feb 2017
 *      Author: gnx91527
 */
#include <string>
#include <list>
#include <fstream>

#ifndef SRC_ASCIISTACKREADER_H_
#define SRC_ASCIISTACKREADER_H_

class AsciiStackReader {
public:
	AsciiStackReader(const std::string& path);
	virtual ~AsciiStackReader();
	std::string readNext();

private:
	std::ifstream currentFile_;
	std::list<std::string> files_;
	std::list<std::string> buffer_;
};

#endif /* SRC_ASCIISTACKREADER_H_ */
