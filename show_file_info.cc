#include <iostream>
#include <inttypes.h>
#include <stdint.h>
#include <fstream>
#include <iomanip>
#include <vector>

#include "FileSinkHeader.h"
#include "FileSinkTrailer.h"

int print_file_infos (std::vector<std::ifstream> &ifs, std::vector<std::string>& filenames, std::vector<uint64_t> &filesizes) {
  for (int ifile = 0; ifile < ifs.size(); ifile++) {
    std::ifstream::pos_type beg = ifs[ifile].tellg();
    FileSinkTrailer::Trailer fileTrailer;
    ifs[ifile].seekg(filesizes[ifile]-sizeof(fileTrailer), std::ios_base::beg);
    ifs[ifile].read((char*)&fileTrailer,sizeof(fileTrailer));

    struct tm *dt;
    char buffer [40];
    dt = localtime(&fileTrailer.startUnixtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", dt);
    std::string startTime = buffer;
    dt = localtime(&fileTrailer.stopUnixtime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", dt);
    std::string stopTime = buffer;
    std::cout << "file name: " << filenames[ifile]
	      << ", file size: " << filesizes[ifile]
	      << ", start: " << startTime << " (" << fileTrailer.startUnixtime << ")"
	      << ", stop: " << stopTime << " (" << fileTrailer.stopUnixtime << ")"
	      << std::endl;
    ifs[ifile].seekg(beg, std::ios_base::beg);
  }
  return 0;
}

int get_file_sizes ( std::vector<std::ifstream>& ifs, std::vector<uint64_t> &filesizes) {
  if (ifs.size() != filesizes.size()){
    filesizes.resize(ifs.size());
  }
  for (int ifile = 0; ifile < ifs.size(); ifile++) {
    std::ifstream::pos_type beg = ifs[ifile].tellg();
    ifs[ifile].seekg(0, std::ios::end);
    std::ifstream::pos_type fsize = ifs[ifile].tellg();
    filesizes[ifile] = fsize;
    ifs[ifile].seekg(beg, std::ios_base::beg);
  }
  return 0;
}

int main(int argc, char* argv[]){
  if (argc <= 1) {
    std::cout << "Usage: ./file_info file1 [file2 file3 ...]" << std::endl;
    return 1;
  }
  std::vector<std::string>   filenames;
  std::vector<std::ifstream> ifs;
  std::vector<uint64_t>      filesizes;
  for (unsigned int i = 1; i < argc; i++) {
    filenames.push_back(argv[i]);
  }
  for (unsigned int i = 0; i < filenames.size(); i++) {
    ifs.emplace_back(filenames[i].c_str(), std::ios::binary); /* emplace_back for std::ifstream is valid in C++11 or higher version. */
  }
  get_file_sizes(ifs, filesizes);
  print_file_infos(ifs, filenames, filesizes);
  return 0;
}
