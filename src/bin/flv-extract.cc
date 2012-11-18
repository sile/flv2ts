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

using namespace flv2ts;

// XXX:
#include <inttypes.h>
struct adts_header {
  uint16_t syncword:12; // always set to '0b11111111'
  uint8_t  id:1;        // 0: MPEG-4, 1: MPEG-2
  uint8_t  layer:2;     // always set to '0b00'
  uint8_t  protection_absent:1;
  uint8_t  profile:2; // 00:MAIN, 01:LC, 10:SSR, 11:reserved
  uint8_t  sampling_frequency_index:4;
  uint8_t  private_bit:1;
  uint8_t  channel_configuration:3; 
  uint8_t  original_copy:1;  // 0:original, 1:copy
  uint8_t  home:1;
  uint8_t  copyright_identification_bit:1;
  uint8_t  copyright_identification_start:1;
  uint16_t aac_frame_length:13; // length of the frame incl. header
  uint16_t adts_buffer_fullness:11; // 0x7FF indicates VBR
  uint8_t  no_raw_data_blocks_in_frame:2;
  uint16_t crc_check;  // only if protetion_absent == 0
};

/*
  sampling_frequency_index:
   0: 96000
   1: 88200
   2: 64000
   3: 48000
   4: 44100
   5: 32000
   6: 24000
   7: 22050
   8: 16000
   9: 12000
  10: 11025
  11: 8000
 */

adts_header make_adts_header(size_t aac_payload_length) {
  adts_header h;
  h.syncword = 0xFFF;
  h.id = 0; // MPEG-4
  h.layer = 0;
  h.protection_absent = 1;
  h.profile = 1; // LC
  h.sampling_frequency_index = 4; // 44100 XXX:
  h.private_bit = 0;
  h.channel_configuration = 2;
  h.original_copy = 0;
  h.home = 0;
  h.copyright_identification_bit = 0;
  h.copyright_identification_start = 0;
  h.aac_frame_length = aac_payload_length + 7;
  h.adts_buffer_fullness = 0x7FF;
  h.no_raw_data_blocks_in_frame = 0;
  
  return h;
}

void dump_adts_header(const adts_header& h, char* buf) {
  buf[0] = (h.syncword & 0xFF0) >> 4;

  buf[1] = (((h.syncword & 0x00F) << 4) +
            (h.id << 3) +
            (h.layer << 1) +
            (h.protection_absent));

  buf[2] = ((h.profile << 6) +
            (h.sampling_frequency_index << 2) +
            (h.private_bit << 1) +
            (h.channel_configuration >> 2));

  buf[3] = ((h.channel_configuration << 6) +
            (h.original_copy << 5) +
            (h.home << 4) +
            (h.copyright_identification_bit << 3) +
            (h.copyright_identification_start << 2) +
            (h.aac_frame_length >> 11));
  
  buf[4] = (h.aac_frame_length >> 3);
  
  buf[5] = ((h.aac_frame_length << 5) +
            (h.adts_buffer_fullness >> 6));

  buf[6] = ((h.adts_buffer_fullness << 2) + 
            h.no_raw_data_blocks_in_frame);
}


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
        if(tag.audio.sound_format != 10) {
          std::cerr << "unsupported audio format: " << tag.audio.sound_format << std::endl;
          return 1;
        }
        if(tag.audio.aac_packet_type == 0) {
          // AudioSpecificConfig
          continue;
        }
        
        adts_header adts = make_adts_header(tag.audio.payload_size);
        char buf[7];
        dump_adts_header(adts, buf);
        std::cout.write(buf, 7);
        std::cout.write(reinterpret_cast<const char*>(tag.audio.payload), tag.audio.payload_size);
      }
      break;
    case flv::Tag::TYPE_VIDEO:
      if(! audio) {
        std::cout.write(reinterpret_cast<const char*>(tag.video.payload), tag.video.payload_size);        
      }
      break;
    default:
      break;
    }
  }

  return 0;
}
