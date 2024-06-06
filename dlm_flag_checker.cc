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
    std::cout << "Usage: ./dlm_flag_checker filename " << std::endl;
    return 1;
  }
  std::string   filename = argv[1];
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  FileSinkHeader::Header fileHeader;
  ifs.read((char*)&fileHeader,sizeof(fileHeader));
  
  std::map<uint64_t, std::vector<int> > hbflag_count;
  std::map<uint64_t, std::string > femId_ip;
  
  std::map<uint64_t, uint64_t> femScaler;
  std::map<uint64_t, uint64_t> femScaler2;

  while( !ifs.eof() && (ifs.tellg() != -1) ){ 
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    char * magic_char = (char*) &magic;
    magic_char[8] = (char)0;
    //std::cout << "Magic: " << magic_char << std::endl;
    
    switch (magic) {
    case TimeFrame::MAGIC: {
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
      //std::cout << "TimeFrameId: " << std::dec << tfbHeader.timeFrameId << std::hex << " " << tfbHeader.timeFrameId << std::endl;
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
      if (femScaler.count(stfHeader.femId) == 0) {
	femScaler.insert(std::make_pair(stfHeader.femId, 0));
	femScaler2.insert(std::make_pair(stfHeader.femId, 0));
      }
      
      //std::cout << "  ================="<< std::endl;
      //std::cout << "  timeframe_id: " << std::hex << stfHeader.timeFrameId << std::dec << std::endl;
      //std::cout << "  femType     : " << std::hex << stfHeader.femType << std::dec << std::endl;
      //std::cout << "  femId       : " << std::hex << stfHeader.femId << std::dec << std::endl;
      //std::cout << "  femId (ip)  : " << femId_ip[stfHeader.femId] << std::endl;
      //std::cout << "  length      : " << stfHeader.length << std::endl;
      //std::cout << "  hLength     : " << stfHeader.hLength << std::endl;	
      //std::cout << "  timeSec     : " << stfHeader.timeSec << std::endl;
      //std::cout << "  timeUsec    : " << stfHeader.timeUSec << std::endl;  
      //std::cout << "  # of words  : " << stfHeader.length / sizeof(uint64_t) << std::endl;      
      
      unsigned int nword = (stfHeader.length - sizeof(stfHeader)) / 8;
      //std::cout << "nword: " << nword << std::endl;
      uint64_t hbcounter = 0;
      for(unsigned int i=0; i< nword; i++){
	AmQStrTdc::Data::Bits idata;
	ifs.read((char*)&idata, sizeof(idata));
	if(idata.head == AmQStrTdc::Data::Heartbeat) {
	  //std::cout << "data: 0x" << std::hex << idata.head << std::endl;
	  //uint64_t aa = AmQStrTdc::Data::Heartbeat;
	  //std::cout << "AmQStrTdc::Data::Heartbeat: 0x" << std::hex << aa << std::endl;
	  
	  if (hbflag_count.count(stfHeader.femId) == 0) {
	    hbflag_count.insert(std::make_pair(stfHeader.femId, std::vector<int>(17)));
	  }
	  hbflag_count[stfHeader.femId][0]++;
	  for (int iflag = 1; iflag < 17; iflag++) {
	    bool flag_bool = idata.hbflag >> (iflag-1) & 0x1;
	    if (flag_bool) {
	      uint64_t pos = (uint64_t)ifs.tellg() - sizeof(idata);
	      std::cout << "Pos (bytes): " << pos
			<< " (0o" << std::oct << pos << std::dec
			<< "), HBF num: " << idata.hbframe
			<< ", femId: (0x" << std::hex << stfHeader.femId << std::dec
			<< ", " << femId_ip[stfHeader.femId]
			<< "), bit: " << iflag
			<< std::endl;
	      hbflag_count[stfHeader.femId][iflag] += 1;
	    }
	  }
	  hbcounter +=1;
	  //}else if (idata.head == Data::Data || idata.head == Data::Trailer ||
	  //    idata.head == Data::ThrottlingT1Start || idata.head == Data::ThrottlingT1End ||
	  //    idata.head == Data::ThrottlingT2Start || idata.head == Data::ThrottlingT2End){
	  //  femScaler[stfHeader.femId] += 1;
	  //}
	}else if (idata.head == AmQStrTdc::Data::Data){
	  femScaler[stfHeader.femId] += 1;
	}
      }
      femScaler2[stfHeader.femId] += (nword - hbcounter*4);
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
  std::cout << "data file: "<< filename << std::endl;
  //std::cout << "hbflag_count.begin()->second.size(): "<< hbflag_count.begin()->second.size() << std::endl;

  std::cout << std::setw(13) << "ip address";
  for (int ibit =1; ibit < hbflag_count.begin()->second.size(); ibit++) {
    std::cout << "|" << std::setw(7) << "bit" + std::to_string(ibit);
  }
  std::cout << "|" << std::setw(7) << "total" << std::endl;
  for (auto ite = hbflag_count.begin(); ite != hbflag_count.end(); ite++) {
    std::cout << std::setw(13) << femId_ip[ite->first];
    for (int ibit =1; ibit < ite->second.size(); ibit++) {
      std::cout << "|" << std::setw(7) << (ite->second)[ibit];
    }
    std::cout << "|" << std::setw(7) << (ite->second)[0];
    std::cout << std::endl;
  }
  for (auto ite = femScaler.begin(); ite != femScaler.end(); ite++) {
    double time_in_sec = hbflag_count[ite->first][0] * 0.000524288;
    std::cout << std::setw(13) << femId_ip[ite->first] << ": ";
    std::cout << std::setw(12) << ite->second <<  " / " <<  std::setw(12) << time_in_sec << " sec = ";
    std::cout << std::setw(12) << (ite->second / time_in_sec) << " cps" << std::endl;
  }
  //for (auto ite = femScaler2.begin(); ite != femScaler2.end(); ite++) {
  //  double time_in_sec = hbflag_count[ite->first][0] * 0.000524;
  //  std::cout << std::setw(13) << femId_ip[ite->first] << ": ";
  //  std::cout << std::setw(12) << ite->second <<  " / " <<  std::setw(12) << time_in_sec << " sec = ";
  //  std::cout << std::setw(12) << (ite->second / time_in_sec) << " cps" << std::endl;
  //}
}
