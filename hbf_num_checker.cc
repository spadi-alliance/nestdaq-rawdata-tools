#include <iostream>
#include <inttypes.h>
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include <vector>
#include <map>

#include "FileSinkHeader.h"
#include "FileSinkTrailer.h"
#include "AmQStrTdcData.h"
#include "SubTimeFrameHeader.h"
#include "TimeFrameHeader.h"
#include "FilterHeader.h"

int read_data(std::ifstream &ifs, uint64_t fsize, uint64_t currTimeFrameId, uint64_t nextTimeFrameId,
	      uint64_t& lastTimeFrameId, std::map <uint64_t, uint64_t>& currHeartBeatNumber){  
  //std::cout << "currTimeFrameId: " << currTimeFrameId << std::endl;
  while( !ifs.eof() && (ifs.tellg() != -1) ){  
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    switch (magic) {
    case TimeFrame::MAGIC: {
      std::ifstream::pos_type pos = ifs.tellg();
      static int prev_read_ratio = 0;
      int curr_read_ratio = (double) pos / fsize * 10;
      if (  curr_read_ratio > prev_read_ratio ) {
	std::cout << "Read: " <<  curr_read_ratio * 10 << " % ("<< pos/1000000 << " MB / " << fsize/1000000 << " MB)" << std::endl;
	prev_read_ratio = curr_read_ratio;
      }
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
      //std::cout << "TimeFrameId: " << tfbHeader.timeFrameId << std::endl;
      if ( tfbHeader.timeFrameId > nextTimeFrameId ){
	lastTimeFrameId = tfbHeader.timeFrameId;
	return 0;
      }
      currTimeFrameId = tfbHeader.timeFrameId;
      break;}
    case SubTimeFrame::MAGIC: {
      SubTimeFrame::Header stfHeader;
      ifs.read((char*)&stfHeader, sizeof(stfHeader));
      //std::cout << "  ================="<< std::endl;
      //std::cout << "  timeframe_id: "  << stfHeader.timeFrameId << std::endl;
      //std::cout << "  femType     : " << std::hex << stfHeader.femType << std::dec << std::endl;
      //std::cout << "  femId       : " << std::hex << stfHeader.femId << std::dec << std::endl;
      //std::cout << "  femId (ip)  : " << femId_ip[stfHeader.femId] << std::endl;
      //std::cout << "  length      : " << stfHeader.length << std::endl;
      //std::cout << "  hLength     : " << stfHeader.hLength << std::endl;	
      //std::cout << "  timeSec     : " << stfHeader.timeSec << std::endl;
      //std::cout << "  timeUsec    : " << stfHeader.timeUSec << std::endl;  
      //std::cout << "  # of words : " << stfHeader.length / sizeof(uint64_t) << std::endl;      
      
      unsigned int nword = (stfHeader.length / 8) - (sizeof(SubTimeFrame::Header) / 8);
      for(unsigned int i=0; i < nword; i++){
	AmQStrTdc::Data::Bits idata;
	ifs.read((char*)&idata, sizeof(uint64_t));
	if(idata.head == AmQStrTdc::Data::Heartbeat) {
	  if (currHeartBeatNumber.count(stfHeader.femId) == 0) {
	    currHeartBeatNumber.insert(std::make_pair(stfHeader.femId, (uint64_t)idata.hbframe));
	  }else{
	    if (stfHeader.femId == 0xc0a80274) {
	      std::cout << "TimeFrameId: " <<  currTimeFrameId;
	      std::cout << ", femId: " << std::hex << stfHeader.femId << std::dec;
	      std::cout << ", prev. hbframe number: "<< currHeartBeatNumber[stfHeader.femId];
	      std::cout << ", curr. hbframe number: "<< idata.hbframe << std::endl;
	      if ((currHeartBeatNumber[stfHeader.femId] + 1) != idata.hbframe) {
		std::cout << " TimeFrameId: " <<  currTimeFrameId;
		std::cout << ", femId: " << std::hex << stfHeader.femId << std::dec;
		std::cout << ", prev. hbframe number: "<< currHeartBeatNumber[stfHeader.femId];
		std::cout << ", curr. hbframe number: "<< idata.hbframe << std::endl;
	      }
	    }
	    currHeartBeatNumber[stfHeader.femId] = idata.hbframe;
	    //std::cout << std::setw(7) << currTimeFrameId;
	    //for (auto ite = currHeartBeatNumber.begin(); ite != currHeartBeatNumber.end(); ite++) {
	    //  std::cout << "|" << std::setw(7) << ite->second;
	    //}
	    //std::cout << std::endl;
	  }
	}
      }
      break;}
    case FileSinkTrailer::MAGIC: {
      ifs.seekg(sizeof(FileSinkTrailer::Trailer), std::ios_base::cur);
      break;}
    case Filter::MAGIC: {
      ifs.seekg(sizeof(Filter::Header), std::ios_base::cur);
      break;}
    default: {
      ifs.read((char*)&magic, sizeof(magic));
      uint32_t length;
      ifs.read((char*)&length, sizeof(length));
      ifs.seekg(length - sizeof(length) - sizeof(magic), std::ios_base::cur);
      //std::cout << "length: " << length << std::endl;
      break;}
    }
  }
  return 1;
}

int main(int argc, char* argv[]){
  if (argc <= 1) {
    std::cout << "Usage: ./hbf_num_checker file1 [file2 file3 ...]" << std::endl;
    return 1;
  }
  std::vector<std::string>   filenames;
  std::vector<std::ifstream> ifs;
  std::vector<uint64_t>      filesizes;
  for (int i = 1; i < argc; i++) {
    filenames.push_back(argv[i]);
  }
  for (unsigned int i = 0; i < filenames.size(); i++) {
    ifs.emplace_back(filenames[i].c_str(), std::ios::binary); /* emplace_back for std::ifstream is valid in C++11 or higher version. */
  }
  
  std::multimap <uint64_t, unsigned int> headOfTimeFrameId;
  uint64_t lastTimeFrameId;
  std::vector <uint64_t> currTimeFrameId(ifs.size(),0);
  std::map <uint64_t, uint64_t> currHeartBeatNumber;
  
  for (unsigned int ifile = 0; ifile < ifs.size(); ifile++){
    FileSinkHeader::Header fileHeader;
    ifs[ifile].read((char*)&fileHeader,sizeof(fileHeader));
    read_data(ifs[ifile], filesizes[ifile], 0, 0, lastTimeFrameId, currHeartBeatNumber);
    headOfTimeFrameId.insert(std::make_pair(lastTimeFrameId, ifile));
  }
  while (true) {
//    std::cout <<  "headOfTimeFrameId.begin()->first" << headOfTimeFrameId.begin()->first << std::endl;
//    std::cout <<  "headOfTimeFrameId.begin()->second" << headOfTimeFrameId.begin()->second << std::endl;
//    std::cout <<  "(++headOfTimeFrameId.begin())->first" << (++headOfTimeFrameId.begin())->first << std::endl;
//    std::cout <<  "(++headOfTimeFrameId.begin())->second" << (++headOfTimeFrameId.begin())->second << std::endl;
    uint64_t     currTimeFrameId = headOfTimeFrameId.begin()->first;
    unsigned int ifile           = headOfTimeFrameId.begin()->second;
    if (headOfTimeFrameId.size()==1) {
      read_data(ifs[ifile], filesizes[ifile],
		currTimeFrameId, 0xffffffff,
		lastTimeFrameId, currHeartBeatNumber);
      break;
    }else{
      uint64_t nextTimeFrameId = (++headOfTimeFrameId.begin())->first;
      //      std::cout << "ifile: " << ifile <<std::endl;
      int ret = read_data(ifs[ifile], filesizes[ifile],
			  currTimeFrameId, nextTimeFrameId,
			  lastTimeFrameId, currHeartBeatNumber);
      headOfTimeFrameId.erase(headOfTimeFrameId.begin());
//      std::cout << "currTimeFrameId: " << currTimeFrameId <<std::endl;
//      std::cout << "nextTimeFrameId: " << nextTimeFrameId <<std::endl;
//      std::cout << "lastTimeFrameId: " << lastTimeFrameId <<std::endl;
      if (ret != 1) {
        headOfTimeFrameId.insert(std::make_pair(lastTimeFrameId, ifile));
      }
    }
  }
  return 0;
}
