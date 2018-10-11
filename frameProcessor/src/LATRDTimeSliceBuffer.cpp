//
// Created by gnx91527 on 17/09/18.
//
#include "LATRDTimeSliceBuffer.h"
#include <stdio.h>
namespace FrameProcessor {

LATRDTimeSliceBuffer::LATRDTimeSliceBuffer()
{
  // Setup logging for the class
  logger_ = Logger::getLogger("FP.LATRDTimeSliceBuffer");
  logger_->setLevel(Level::getAll());
  LOG4CXX_DEBUG(logger_, "LATRDTimeSliceBuffer constructor.");
}

LATRDTimeSliceBuffer::~LATRDTimeSliceBuffer()
{
  job_store_.clear();
}

void LATRDTimeSliceBuffer::add_job(boost::shared_ptr<LATRDProcessJob> job)
{
  // Verify that the packet ID does not already exist within this buffer
  if (job_store_.count(job->packet_number) > 0){
    LOG4CXX_ERROR(logger_, "Duplicate packet detected for packet ID [" << job->packet_number << "]");
    throw LATRDProcessingException("Duplicate packet ID for the same time slice");
  } else {
    job_store_[job->packet_number] = job;
  }
}

std::vector<boost::shared_ptr<LATRDProcessJob> > LATRDTimeSliceBuffer::empty()
{
  std::vector<boost::shared_ptr<LATRDProcessJob> > jobs;
  std::map<uint32_t, boost::shared_ptr<LATRDProcessJob> >::iterator iter;
  for (iter = job_store_.begin(); iter != job_store_.end(); ++iter){
    jobs.push_back(iter->second);
  }
  job_store_.clear();
  return jobs;
}

size_t LATRDTimeSliceBuffer::size()
{
  return job_store_.size();
}

std::string LATRDTimeSliceBuffer::report()
{
  // Print a full report of wrap object
  std::stringstream ss;
  ss << "***  => Number of packets stored: " << job_store_.size() << "\n" << "***  => ";
  std::map<uint32_t, boost::shared_ptr<LATRDProcessJob> >::iterator iter;
  int counter = 0;
  for (iter = job_store_.begin(); iter != job_store_.end(); ++iter){
    ss << iter->first << ", ";
    counter++;
    if (counter % 10 == 0){
      ss << "\n***  => ";
    }
  }
  ss << "\n";
  return ss.str();
}

}
