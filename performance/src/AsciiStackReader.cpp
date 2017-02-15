/*
 * AsciiStackReader.cpp
 *
 *  Created on: 6 Feb 2017
 *      Author: gnx91527
 */

#include "AsciiStackReader.h"
#include <fstream>
#include <iostream>
#include <string>

#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

AsciiStackReader::AsciiStackReader(const std::string& path)
{
	std::string fpath;
	DIR *dir;
	struct dirent *ent;
	if (path.find("/", path.length()-1) == std::string::npos){
		fpath = path + "/";
	}
	if ((dir = opendir (fpath.c_str())) != NULL){
		while ((ent = readdir (dir)) != NULL) {
			if (ent->d_type == 8){
				std::string fname = fpath + std::string(ent->d_name);
				files_.push_back(fname);
			}
		}
		closedir (dir);
		printf ("Parsing new file %s\n", files_.front().c_str());
		currentFile_.open(files_.front().c_str());
		files_.pop_front();
	} else {
		// could not open directory
		perror ("");
	}
}

AsciiStackReader::~AsciiStackReader() {
	// TODO Auto-generated destructor stub
}

std::string AsciiStackReader::readNext()
{
	std::string line;
	while (buffer_.empty()){
		// Buffer is empty, add some more content
		int counter = 0;
		while (std::getline(currentFile_, line) && counter < 10000){
			buffer_.push_back(line);
			counter++;
		}
		if (counter < 10000){
			currentFile_.close();
			printf ("Parsing new file %s\n", files_.front().c_str());
			currentFile_.open(files_.front().c_str());
			files_.pop_front();
		}
	}
	std::string value = buffer_.front();
	buffer_.pop_front();
	return value;
}
