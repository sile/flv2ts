#include <flv/parser.hh>
#include <iostream>
#include <inttypes.h>

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
    return 1;
  }

  // header
  std::cout << "[header]" << std::endl
            << "  signature:   " << header.signature[0] << header.signature[1] << header.signature[2] << std::endl
            << "  version:     " << (int)header.version << std::endl
            << "  is_audio:    " << (header.is_audio ? "true" : "false") << std::endl
            << "  is_video:    " << (header.is_video ? "true" : "false") << std::endl
            << "  data_offset: " << header.data_offset << std::endl
            << std::endl;
  parser.abs_seek(header.data_offset);

  // body
  for(int i=0;; i++) {
    size_t offset = parser.position();
    flv2ts::flv::Tag tag;
    uint32_t prev_tag_size;
    if(! parser.parseTag(tag, prev_tag_size)) {
      std::cerr << "parse flv tag failed" << std::endl;
      return 1;
    }
    
    if(parser.eos()) {
      break;
    }
    
    std::cout << "[tag:" << i << ":" << offset << "]" << std::endl
              << "  filter:    " << (tag.filter ? "true" : "false") << std::endl
              << "  type:      " << (int)tag.type << std::endl
              << "  data_size: " << tag.data_size << std::endl
              << "  timestamp: " << tag.timestamp << std::endl
              << "  stream_id: " << tag.stream_id << std::endl;
    switch(tag.type) {
    case flv2ts::flv::Tag::TYPE_SCRIPT_DATA: {
      std::cout << "  [script_data]" << std::endl
                << "    payload_size: " << tag.data_size << std::endl;
      break;
    }
    case flv2ts::flv::Tag::TYPE_AUDIO: {
      const flv2ts::flv::AudioTag& audio = *reinterpret_cast<flv2ts::flv::AudioTag*>(tag.data);
      std::cout << "  [audio]" << std::endl
                << "    sound_format: " << (int)audio.sound_format << std::endl
                << "    sound_rate:   " << (int)audio.sound_rate << std::endl
                << "    sound_size:   " << (int)audio.sound_size << std::endl
                << "    sound_type:   " << (int)audio.sound_type << std::endl;
      if(audio.sound_format == 10) {
        std::cout << "    acc_packate_type: " << (int)audio.aac_packet_type << std::endl;
      }
      std::cout << "    payload_size: " << audio.payloadSize(tag) << std::endl;
    }
    default:
      std::cout << std::endl;      
    }
    std::cout << std::endl;
  }
  
  return 0;
}
