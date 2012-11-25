#include <iostream>
#include <fstream>
#include <flv/parser.hh>
#include <adts/header.hh>
#include <ts/packet.hh>
#include <aux/crc32.hh>
#include <stdlib.h>
#include <string.h>

#include <h264/avc_decoder_configuration_record.hh>
#include <h264/avc_sample.hh>

using namespace flv2ts;

int main(int argc, char** argv) {
  if(argc != 4) {
    std::cerr << "Usage: build-hls-index INPUT_FLV_FILE OUTPUT_PREFIX DURATION" << std::endl;
    return 1;
  }
  
  const char* flv_file = argv[1];
  const std::string output_prefix = argv[2];
  const unsigned duration = atoi(argv[3]);
  
  std::fstream m3u8((output_prefix + ".m3u8").c_str(), std::ios::out);
  if(! m3u8) {
    std::cerr << "Can't open file: " << output_prefix + ".m3u8" << std::endl;
    return 1;
  }

  std::fstream ts_index((output_prefix + ".ts_idx").c_str(), std::ios::out|std::ios::binary);
  if(! ts_index) {
    std::cerr << "Can't open file: " << output_prefix + ".ts_idx" << std::endl;
    return 1;
  }

  flv2ts::flv::Parser flv(flv_file);
  if(! flv) {
    std::cerr << "Can't open file: " << flv_file << std::endl;
    return 1;
  }
  
  // flv header
  flv::Header flv_header;
  if(! flv.parseHeader(flv_header)) {
    std::cerr << "parse flv header failed" << std::endl;
    return 1;
  }
  flv.abs_seek(flv_header.data_offset);
  
  // output m3u8 header
  m3u8 << "#EXTM3U" << std::endl
       << "#EXT-X-TARGETDURATION:" << duration << std::endl
       << "#EXT-X-MEDIA-SEAUENCE:0" << std::endl
       << std::endl;

  unsigned ts_seq=0;
  bool switched=true;

  // flv body
  uint64_t next_timestamp = 0;
  for(size_t kk=0;; kk++) {
    flv::Tag tag;
    uint32_t prev_tag_size;
    uint32_t prev_position = flv.position();
    if(! flv.parseTag(tag, prev_tag_size)) {
      std::cerr << "parse flv tag failed" << std::endl;
      return 1;
    }
    
    if(flv.eos()) {
      break;
    }

    if(next_timestamp <= static_cast<uint64_t>(tag.timestamp) &&
       tag.type == flv::Tag::TYPE_VIDEO &&
       tag.video.codec_id == 7 &&
       tag.video.avc_packet_type == 1) {
      if(tag.video.frame_type != flv::VideoTag::FRAME_TYPE_KEY) {
        std::cerr << "[warn] not key frame: timestamp=" << tag.timestamp << std::endl;
      }
      switched = true;
    }

    if(switched) {
      ts_index.write(reinterpret_cast<const char*>(&prev_position), sizeof(uint32_t)); // XXX: native-endian

      m3u8 << "#EXTINF:" << duration << std::endl
           << basename(output_prefix.c_str()) << "-" << ts_seq << ".ts" << std::endl 
           << std::endl;
      
      switched = false;
      ts_seq++;
      next_timestamp = tag.timestamp + duration * 1000;
    }

    switch(tag.type) {
    case flv::Tag::TYPE_AUDIO: {
      // audio
      break;
    }
      
    case flv::Tag::TYPE_VIDEO: {
      // video
      if(tag.video.codec_id != 7) { // 7=AVC
        std::cerr << "unsupported video codec: " << tag.video.codec_id << std::endl;
        return 1;
      }

      switch(tag.video.avc_packet_type) {
      case 0: {
        // AVC sequence header
        h264::AVCDecoderConfigurationRecord conf;
        aux::ByteStream conf_in(tag.video.payload, tag.video.payload_size);
        if(! conf.parse(conf_in)) {
          std::cerr << "parse AVCDecoderConfigurationRecord failed" << std::endl;
          return 1;
        }
        
        std::fstream conf_out((output_prefix + ".avc_conf").c_str(), std::ios::out|std::ios::binary);
        if(! conf_out) {
          std::cerr << "Can't open file: " << output_prefix + ".avc_conf" << std::endl;
          return 1;
        }
        conf_out.write(reinterpret_cast<const char*>(tag.video.payload), tag.video.payload_size);
        break;
      }
      case 1: {
        // AVC NALU
        break;
      }
      case 2: {
        // AVC end of sequence
        break;
      }
      default: {
        std::cerr << "unknown avc_packet_type: " << tag.video.avc_packet_type << std::endl;
        return 1;
      }
      }
    }
    default:
      break;
    }
  }
  uint32_t end_position = flv.position();
  ts_index.write(reinterpret_cast<const char*>(&end_position), sizeof(uint32_t)); // XXX: native-endian

  // output m3u8 tail
  m3u8 << "#EXT-X-ENDLIST" << std::endl;
  
  return 0;
}
