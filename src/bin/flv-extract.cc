// RTP Payload Format for Transport of MPEG-4 Elementary Streams
// - http://tools.ietf.org/html/rfc3640
//
// - http://en.wikipedia.org/wiki/Advanced_Audio_Coding
//
// - adts: http://developer.longtailvideo.com/trac/export/1460/providers/adaptive/doc/adts.pdf
//
#include <iostream>
#include <string.h>
#include <flv/parser.hh>
#include <adts/header.hh>

using namespace flv2ts;

int main(int argc, char** argv) {
  if(argc != 3) {
    std::cerr << "Usage: flv-extract audio|video TS_FILE" << std::endl;
    return 1;
  }

  const char* type = argv[1];
  if(strcmp(type, "audio") != 0 &&
     strcmp(type, "video") != 0) {
    std::cerr << "Undefined type: " << type << std::endl;
    return 1;
  }
  const bool audio = strcmp(type, "audio") == 0;

  const char* filepath = argv[2];
  flv::Parser parser(filepath);
  if(! parser) {
    std::cerr << "Can't open file: " << filepath << std::endl;
    return 1;
  }

  // flv header
  flv::Header header;
  if(! parser.parseHeader(header)) {
    std::cerr << "parse flv header failed" << std::endl;
    return 1;
  }
  parser.abs_seek(header.data_offset);

  // flv body
  for(;;) {
    flv::Tag tag;
    uint32_t prev_tag_size;
    if(! parser.parseTag(tag, prev_tag_size)) {
      std::cerr << "parse flv tag failed" << std::endl;
      return 1;
    }
    
    if(parser.eos()) {
      break;
    }

    switch(tag.type) {
    case flv::Tag::TYPE_AUDIO:
      if(audio) {
        if(tag.audio.sound_format != 10) { // 10=AAC
          std::cerr << "unsupported audio format: " << tag.audio.sound_format << std::endl;
          return 1;
        }
        if(tag.audio.aac_packet_type == 0) {
          // AudioSpecificConfig
          continue;
        }
        
        adts::Header adts = adts::Header::make_default(tag.audio.payload_size);
        char buf[7];
        adts.dump(buf, 7);
        std::cout.write(buf, 7);
        std::cout.write(reinterpret_cast<const char*>(tag.audio.payload), tag.audio.payload_size);
      }
      break;
    case flv::Tag::TYPE_VIDEO:
      if(! audio) {
        if(tag.video.codec_id != 7) { // 7=AVC
          std::cerr << "unsupported video codec: " << tag.video.codec_id << std::endl;
          return 1;
        }
        if(tag.video.avc_packet_type != 1) {
          // not AVC NALU
          continue;
        }

        std::cout.write(reinterpret_cast<const char*>(tag.video.payload), tag.video.payload_size);        
      }
      break;
    default:
      break;
    }
  }

  return 0;
}
