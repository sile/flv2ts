#include <iostream>
#include <flv/parser.hh>

int main(int argc, char** argv) {
  if(argc != 2) {
    std::cerr << "Usage: parse-flv FLV_FILE" << std::endl;
    return 1;
  }

  const char* filepath = argv[1];
  
  flv2ts::flv::Parser parser(filepath);
  if(! parser) {
    std::cerr << "Can't open file: " << filepath << std::endl;
  }

  return 0;
}
