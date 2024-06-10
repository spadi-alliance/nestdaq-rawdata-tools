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

int main(int argc, char* argv[]){
  if (argc <= 1) {
    std::cout << "Usage: ./scaler_checker filename " << std::endl;
    return 1;
  }
  std::string   filename = argv[1];
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::ifstream::pos_type beg = ifs.tellg();
  ifs.seekg(0, std::ios::end);
  std::ifstream::pos_type fsize = ifs.tellg();
  ifs.seekg(beg, std::ios_base::beg);
  FileSinkHeader::Header fileHeader;
  ifs.read((char*)&fileHeader,sizeof(fileHeader));
  std::cout << "Data file: "<< filename
	    << ", size: " << fsize / 1024 / 1024
	    << " MB" << std::endl;
  
  std::map<uint64_t, std::vector<uint64_t> > counter;
  std::map<uint64_t, uint64_t> hbcounter;
  std::map<uint64_t, std::string> femId_ip;
  while( !ifs.eof() && (ifs.tellg() != -1) ){ 
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    char *magic_char = (char*) &magic;
    magic_char[8] = (char)0;
    switch (magic) {
    case TimeFrame::MAGIC: {
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
      std::ifstream::pos_type pos = ifs.tellg();
      static int prev_read_ratio = 0;
      int curr_read_ratio = (double) pos / fsize * 10;
      if (  curr_read_ratio > prev_read_ratio ) {
	std::cout << "Read: " <<  curr_read_ratio * 10 << " % ("<< pos/1024/1024 << " MB / " << fsize/1024/1024 << " MB)" << std::endl;
	prev_read_ratio = curr_read_ratio;
      }
      break;}
    case SubTimeFrame::MAGIC: {
      SubTimeFrame::Header stfHeader;
      ifs.read((char*)&stfHeader, sizeof(stfHeader));
      if (femId_ip.count(stfHeader.femId) == 0) {
	uint8_t* ip_num = (uint8_t*) &(stfHeader.femId);
	std::string ip_address =std::to_string(ip_num[3]) + "."
	  + std::to_string(ip_num[2]) + "."
	  + std::to_string(ip_num[1]) + "."
	  + std::to_string(ip_num[0]);
	femId_ip.insert(std::make_pair(stfHeader.femId, ip_address));
      }
      if (counter.count(stfHeader.femId) == 0) {
	hbcounter.insert(std::make_pair(stfHeader.femId, 0));
	if ( stfHeader.femType == 2 || stfHeader.femType == 5 ) { //HRTDC
	  counter.insert(std::make_pair(stfHeader.femId, std::vector<uint64_t>(64)));
	}else if ( stfHeader.femType == 3 || stfHeader.femType == 6 ) { //LRTDC
	  counter.insert(std::make_pair(stfHeader.femId, std::vector<uint64_t>(128)));
	}
      }
      unsigned int nword = (stfHeader.length - sizeof(stfHeader)) / 8;
      //std::cout << "nword: " << nword << std::endl;
      for(unsigned int i=0; i< nword; i++){
	AmQStrTdc::Data::Bits idata;
	uint64_t pos = (uint64_t)ifs.tellg();
	ifs.read((char*)&idata, sizeof(idata));
	if (idata.head == AmQStrTdc::Data::Heartbeat) {
	  hbcounter[stfHeader.femId]++;
//	}else if (idata.head == AmQStrTdc::Data::Data || idata.head == AmQStrTdc::Data::Trailer ||
//		  idata.head == AmQStrTdc::Data::ThrottlingT1Start || idata.head == AmQStrTdc::Data::ThrottlingT1End ||
//		  idata.head == AmQStrTdc::Data::ThrottlingT2Start || idata.head == AmQStrTdc::Data::ThrottlingT2End){
	}else if (idata.head == AmQStrTdc::Data::Data){
	  if ( stfHeader.femType == 2 || stfHeader.femType == 5 ) { //HRTDC
	    counter[stfHeader.femId][idata.hrch]++;
	  }else if ( stfHeader.femType == 3 || stfHeader.femType == 6 ) { //LRTDC
	    counter[stfHeader.femId][idata.ch]++;
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
      //std::cout << "  length: " << length << std::endl;
      break;}
    }
  }
  
  for (auto ite = hbcounter.begin(); ite != hbcounter.end(); ite++) {
    std::cout << "FemId: " << femId_ip[ite->first] << std::endl;
    double time_in_sec = hbcounter[ite->first] * 0.000524288;
    uint64_t sum = 0;
    for ( int i = 0; i < counter[ite->first].size(); i++) {
      std::cout << std::setw(13) << i << ": ";
      std::cout << std::setw(12) << counter[ite->first][i] <<  " / " <<  std::setw(12) << time_in_sec << " sec = ";
      std::cout << std::setw(12) << (counter[ite->first][i] / time_in_sec) << " cps" << std::endl;
      sum += counter[ite->first][i];
    }
    std::cout << std::setw(13) << "total" << ": ";
    std::cout << std::setw(12) << sum <<  " / " <<  std::setw(12) << time_in_sec << " sec = ";
    std::cout << std::setw(12) << (sum / time_in_sec) << " cps" << std::endl;
  }
  
  return 0;
}
