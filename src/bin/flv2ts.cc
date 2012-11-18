#include <iostream>
#include <flv/parser.hh>

using namespace flv2ts;

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Usage: flv2ts INPUT_FLV_FILE OUTPUT_DIR" << std::endl;
  }

  const char* flv_file = argv[1];
  // const char* output_dir = argv[2];

  flv2ts::flv::Parser flv(flv_file);
  if(! flv) {
    std::cerr << "Can't open file: " << flv_file << std::endl;
  }

  // flv header
  flv::Header flv_header;
  if(! flv.parseHeader(flv_header)) {
    std::cerr << "parse flv header failed" << std::endl;
    return 1;
  }
  flv.abs_seek(flv_header.data_offset);

  // flv body
  for(;;) {
    flv::Tag tag;
    uint32_t prev_tag_size;
    if(! flv.parseTag(tag, prev_tag_size)) {
      std::cerr << "parse flv tag failed" << std::endl;
      return 1;
    }
    
    if(flv.eos()) {
      break;
    }

    switch(tag.type) {
    case flv::Tag::TYPE_AUDIO:
      break;
    case flv::Tag::TYPE_VIDEO:
      break;
    default:
      break;
    }
  }

  return 0;
}
