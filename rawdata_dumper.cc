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
    std::cout << "Usage: ./rawdata_dumper filename " << std::endl;
    return 1;
  }
  std::string   filename = argv[1];
  std::ifstream ifs(filename.c_str(), std::ios::binary);
  std::cout << "Data file: "<< filename << std::endl;
  FileSinkHeader::Header fileHeader;
  ifs.read((char*)&fileHeader,sizeof(fileHeader));
  
  std::map<uint64_t, std::string > femId_ip;
  while( !ifs.eof() && (ifs.tellg() != -1) ){ 
    uint64_t magic;
    ifs.read((char*)&magic, sizeof(magic));
    ifs.seekg(-sizeof(magic), std::ios_base::cur);
    char *magic_char = (char*) &magic;
    magic_char[8] = (char)0;
    std::cout << "POS (bytes): " << std::dec << ifs.tellg()
	      << " (0o" << std::oct << ifs.tellg() << std::dec << ") "
	      << "Magic: " << magic_char;
    switch (magic) {
    case TimeFrame::MAGIC: {
      TimeFrame::Header tfbHeader;
      ifs.read((char*)&tfbHeader, sizeof(tfbHeader));
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
      //std::cout << "nword: " << nword << std::endl;
      uint64_t hbcounter = 0;
      for(unsigned int i=0; i< nword; i++){
	AmQStrTdc::Data::Bits idata;
	uint64_t pos = (uint64_t)ifs.tellg();
	ifs.read((char*)&idata, sizeof(idata));
	if (idata.head == AmQStrTdc::Data::Heartbeat) {
	  std::cout << "POS (bytes): " << std::dec << pos << " (0o" << std::oct << ifs.tellg() << pos << ") "
		    << "  HBF num: 0x" << std::hex << idata.hbframe << std::dec
		    << ", bit: 0x" << std::hex << idata.hbflag << std::dec
		    << std::endl;
	  hbcounter +=1;
	}else if (idata.head == AmQStrTdc::Data::Data || idata.head == AmQStrTdc::Data::Trailer ||
		  idata.head == AmQStrTdc::Data::ThrottlingT1Start || idata.head == AmQStrTdc::Data::ThrottlingT1End ||
		  idata.head == AmQStrTdc::Data::ThrottlingT2Start || idata.head == AmQStrTdc::Data::ThrottlingT2End){
	  std::cout << "POS (bytes): " << std::dec << pos << " (0o" << std::oct << pos << std::dec << ") ";
	  std::cout << "  TDC ";
	  if ( stfHeader.femType == 2 || stfHeader.femType == 5 ) { //HRTDC
	    std::cout << "ch: " << idata.hrch;
	    std::cout << ", tdc: " << idata.hrtdc;
	    std::cout << ", tot: " << idata.hrtot << std::endl;
	  }else if ( stfHeader.femType == 3 || stfHeader.femType == 6 ) { //LRTDC
	    std::cout << "ch: "<< idata.ch;		
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
      break;}
    default: {
      std::cout << "POS (bytes): " << std::dec << ifs.tellg() << " (0o" << std::oct << ifs.tellg() << std::dec << ") ";
      ifs.read((char*)&magic, sizeof(magic));
      uint32_t length;
      ifs.read((char*)&length, sizeof(length));
      ifs.seekg(length - sizeof(length) - sizeof(magic), std::ios_base::cur);
      std::cout << "  length: " << length << std::endl;
      break;}
    }
  }
}
