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

#include "ts_write.hh"
using namespace flv2ts;

int main(int argc, char** argv) {
  if(argc != 4) {
    std::cerr << "Usage: flv2ts INPUT_FLV_FILE OUTPUT_TS_PREFIX DURATION" << std::endl;
    return 1;
  }

  const char* flv_file = argv[1];
  const char* output_ts_prefix = argv[2];
  const unsigned duration = atoi(argv[3]);

  flv2ts::flv::Parser flv(flv_file);
  if(! flv) {
    std::cerr << "Can't open file: " << flv_file << std::endl;
    return 1;
  }
  
  std::cout << "[input]" << std::endl
            << "  flv:      " << flv_file << std::endl
            << "  output:   " << output_ts_prefix << std::endl
            << "  duration: " << duration << std::endl
            << std::endl;

  // flv header
  flv::Header flv_header;
  if(! flv.parseHeader(flv_header)) {
    std::cerr << "parse flv header failed" << std::endl;
    return 1;
  }
  flv.abs_seek(flv_header.data_offset);

  
  unsigned ts_seq=0;
  bool switched=true;
  std::ofstream ts_out;

  // flv body
  tw_state state;
  
  uint64_t next_timestamp = 0;
  h264::AVCDecoderConfigurationRecord conf;
  bool sps_pps_write = false;
  for(size_t kk=0;; kk++) {
    flv::Tag tag;
    uint32_t prev_tag_size;
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
      ts_out.close();

      char buf[1024];
      sprintf(buf, "%s%d.ts", output_ts_prefix, ts_seq);
      ts_out.open(buf, std::ios::out | std::ios::binary);
      if(! ts_out) {
        std::cerr << "Can't open output file: " << buf << std::endl;
        return 1;
      }
      std::cout << "open: " << buf << std::endl;
      
      sps_pps_write = false;
      switched = false;
      ts_seq++;
      next_timestamp = tag.timestamp + duration * 1000;
      write_ts_start(ts_out);
    }

    switch(tag.type) {
    case flv::Tag::TYPE_AUDIO: {
      // audio
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
      
      std::string buf2;
      buf2.assign(buf, 7); // TODO: 既にADTSヘッダが付いているかのチェック
      buf2.append(reinterpret_cast<const char*>(tag.audio.payload), tag.audio.payload_size);
      
      write_audio(state, tag, buf2, ts_out);
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
        aux::ByteStream conf_in(tag.video.payload, tag.video.payload_size);
        if(! conf.parse(conf_in)) {
          std::cerr << "parse AVCDecoderConfigurationRecord failed" << std::endl;
          return 1;
        }
        break;
      }
      case 1: {
        // AVC NALU
        std::string buf;

        if(! sps_pps_write) {
          // TODO: 既に payload に SPS/PPS が含まれているかのチェック
          to_storage_format_sps_pps(conf, buf);  // payloadに SPS/PPS も含める
          sps_pps_write = true;
        }
        
        if(! to_storage_format(conf, tag.video.payload, tag.video.payload_size, buf)) {
          std::cerr << "to_strage_format() failed" << std::endl;
          return 1;
        }
        write_video(state, tag, buf, ts_out);
        
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
  std::cout << std::endl;
  return 0;
}
