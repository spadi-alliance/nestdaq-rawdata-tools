#include <iostream>
#include <inttypes.h>
#include <stdint.h>
#include <cstring>
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

int hbf_sorting (std::ifstream &ifs, uint64_t max_num_read_hbf,
		 std::map<uint32_t, uint64_t>& hbf_sorted_address, uint64_t &read_pos){
  ifs.seekg(read_pos, std::ios_base::beg);
  uint32_t currTimeFrameId = 0;
  uint32_t file_read_flag = 1;
  while(file_read_flag == 1){ 
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    if (ifs.eof() || (ifs.tellg() == -1)) {
      read_pos = ifs.tellg();
      break;
    }
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    switch (magic) {
    case TimeFrame::MAGIC: {
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
      if (tfbHeader.timeFrameId != currTimeFrameId) {
	if (hbf_sorted_address.size() == max_num_read_hbf) {
	  read_pos = (uint64_t) ifs.tellg() - sizeof(TimeFrame::Header);
	  file_read_flag = 0;
	  break;
	}
	currTimeFrameId = tfbHeader.timeFrameId;
	hbf_sorted_address[currTimeFrameId] = (uint64_t) ifs.tellg() - sizeof(TimeFrame::Header);
	//std::cout << "TimeFrameId: " << std::dec << tfbHeader.timeFrameId << std::hex << " 0x" << tfbHeader.timeFrameId << std::endl;
      }
      break;}
    case SubTimeFrame::MAGIC: {
      SubTimeFrame::Header stfHeader;
      ifs.read((char*)&stfHeader, sizeof(stfHeader));
      unsigned int nword = (stfHeader.length - sizeof(stfHeader)) / 8;
      ifs.seekg(nword * sizeof(AmQStrTdc::Data::Bits), std::ios_base::cur);
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
      break;}
    }
  }
  //for (auto ite = hbf_sorted_address.begin(); ite != hbf_sorted_address.end(); ite++) {
  //  std::cout << "Key = " << ite->first << ", Value = " << ite->second << std::endl;
  //}
  return 0;
}

int print_file_position(uint64_t pos) {
  std::cout << "Pos (bytes): " << std::dec << std::setw(11) << pos
	    << " (0o" << std::oct << std::setw(12) << std::setfill('0') << pos << std::setfill(' ') << std::dec << ")| ";
  return 0;
}

int main(int argc, char* argv[]){
  if (argc <= 1) {
    std::cout << "Usage: ./hbf_sorted_rawdata_dumper filename " << std::endl;
    return 1;
  }
  std::string   filename = argv[1];
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::cout << "Data file: "<< filename << std::endl;
  FileSinkHeader::Header fileHeader;
  ifs.read((char*)&fileHeader,sizeof(fileHeader));
  std::map<uint32_t, uint64_t> hbf_sorted_address;
  uint64_t read_pos = ifs.tellg();
  uint64_t max_num_read_hbf = 2000;
  hbf_sorting(ifs, max_num_read_hbf, hbf_sorted_address, read_pos);
  
  std::map<uint64_t, std::string > femId_ip;
  uint32_t timeFrameFlag = 1;
  uint32_t currTimeFrameId = 0;
  uint64_t savedCurrPos = 0;
  while( true ){
    savedCurrPos = ifs.tellg();
    if (timeFrameFlag == 1){
      if (!hbf_sorted_address.empty()) {
	savedCurrPos = hbf_sorted_address.begin()->second;
	currTimeFrameId = hbf_sorted_address.begin()->first;
	hbf_sorted_address.erase(hbf_sorted_address.begin());
	hbf_sorting(ifs, max_num_read_hbf, hbf_sorted_address, read_pos);
      }
      timeFrameFlag = 0;
    }
    ifs.seekg(savedCurrPos, std::ios_base::beg);
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    if (ifs.eof() || (ifs.tellg() == -1)) {
      break;
    }
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    char magic_char[9];
    magic_char[8] = (char)0;
    strncpy(magic_char,(char*)&magic,8);
    
    print_file_position(ifs.tellg());
    std::cout << "Magic: " << magic_char;
    switch (magic) {
    case TimeFrame::MAGIC: {
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
      if (currTimeFrameId != tfbHeader.timeFrameId) {
	timeFrameFlag = 1;
	std::cout << ", TimeFrameId: " << std::dec << tfbHeader.timeFrameId << std::hex << " 0x" << tfbHeader.timeFrameId
		  << ", Jump to the nest TimeFrameId" << std::endl;
	break;
      }
      std::cout << ", TimeFrameId: " << std::dec << tfbHeader.timeFrameId << std::hex << " 0x" << tfbHeader.timeFrameId << std::endl;
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
      std::cout << ", TFID: 0x" << std::hex << stfHeader.timeFrameId << std::dec;
      std::cout << ", FemType: 0x" << std::hex << stfHeader.femType << std::dec;
      std::cout << ", FemId: 0x" << std::hex << stfHeader.femId << std::dec;
      std::cout << ", FemId (ip): " << femId_ip[stfHeader.femId];
      std::cout << ", len: " << stfHeader.length;
      std::cout << ", hLen: " << stfHeader.hLength;	
      std::cout << ", TimeSec: " << stfHeader.timeSec;
      std::cout << ", TimeUsec: " << stfHeader.timeUSec;
      std::cout << ", nwords: " << (stfHeader.length - sizeof(stfHeader)) / 8 << std::endl;      

      unsigned int nword = (stfHeader.length - sizeof(stfHeader)) / 8;
      std::cout << "nword: " << nword << std::endl;
      //uint64_t hbcounter = 0;
      for(unsigned int i=0; i< nword; i++){
	AmQStrTdc::Data::Bits idata;
	uint64_t pos = (uint64_t)ifs.tellg();
	ifs.read((char*)&idata, sizeof(idata));
	if (idata.head == AmQStrTdc::Data::Heartbeat) {
	  print_file_position(pos);
	  std::cout << "   HBF "
		    << "FemId: 0x" << std::hex << std::setw(8) << std::setfill('0') << stfHeader.femId << std::setfill(' ') << std::dec
		    << ", (ip) " << femId_ip[stfHeader.femId]
		    << ", HBF num: 0x" << std::hex << idata.hbframe << std::dec
		    << ", Delimiter flag: (0x" << std::hex << std::setw(4) << std::setfill('0') << idata.hbflag << std::setfill(' ') << std::dec << "";
	  std::cout << ", active bits:";
	  for (int i = 0; i < 16; i++){
	    if ((idata.hbflag >> i) & 0x1) {
	      std::cout << " bit" << i+1;
	    }
	  }
	  if (idata.hbflag == 0){
	    std::cout << " none";
	  }
	  std::cout << ")" << std::endl;
	  //hbcounter +=1;
	}else if (idata.head == AmQStrTdc::Data::Data || idata.head == AmQStrTdc::Data::Trailer ||
		  idata.head == AmQStrTdc::Data::ThrottlingT1Start || idata.head == AmQStrTdc::Data::ThrottlingT1End ||
		  idata.head == AmQStrTdc::Data::ThrottlingT2Start || idata.head == AmQStrTdc::Data::ThrottlingT2End){
	  print_file_position(pos);
	  std::cout << "   TDC ";
	  std::cout << "FemId: 0x" << std::hex << std::setw(8) << std::setfill('0') << stfHeader.femId << std::setfill(' ') << std::dec;
	  std::cout << ", (ip) " << femId_ip[stfHeader.femId];

	  if ( stfHeader.femType == 2 || stfHeader.femType == 5 ) { //HRTDC
	    std::cout << ", ch: " << std::setw(3) << idata.hrch;
	    std::cout << ", tdc: " << idata.hrtdc;
	    std::cout << ", tot: " << idata.hrtot << std::endl;
	  }else if ( stfHeader.femType == 3 || stfHeader.femType == 6 ) { //LRTDC
	    std::cout << ", ch: "<< std::setw(3) << idata.ch;		
	    std::cout << ", tdc: "<< idata.tdc;
	    std::cout << ", tot: "<< idata.tot << std::endl;
	  }
	}
      }
      break;}
    case FileSinkTrailer::MAGIC: {
      ifs.seekg(sizeof(FileSinkTrailer::Trailer), std::ios_base::cur);
      break;}
    case Filter::MAGIC: {
      ifs.seekg(sizeof(Filter::Header), std::ios_base::cur);
      std::cout << std::endl;
      break;}
    default: {
      ifs.read((char*)&magic, sizeof(magic));
      uint32_t length;
      ifs.read((char*)&length, sizeof(length));
      ifs.seekg(length - sizeof(length) - sizeof(magic), std::ios_base::cur);
      std::cout << "  length: " << length << std::endl;
      break;}
    }
  }
}
