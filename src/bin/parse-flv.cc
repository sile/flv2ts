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
  std::cout << "[file]" << std::endl
            << "  path: " << filepath << std::endl
            << std::endl;


  flv2ts::flv::Header header;
  if(! parser.parseHeader(header)) {
    std::cerr << "parse flv header failed" << std::endl;
  }

  std::cout << "[header]" << std::endl
            << "  signature:   " << header.signature[0] << header.signature[1] << header.signature[2] << std::endl
            << "  version:     " << (int)header.version << std::endl
            << "  is_audio:    " << (header.is_audio ? "true" : "false") << std::endl
            << "  is_video:    " << (header.is_video ? "true" : "false") << std::endl
            << "  data_offset: " << header.data_offset << std::endl
            << std::endl;

  return 0;
}
