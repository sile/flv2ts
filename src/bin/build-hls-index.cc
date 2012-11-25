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
  std::fstream null_out("/dev/null", std::ios::out|std::ios::binary); // XXX:

  h264::AVCDecoderConfigurationRecord conf;
  bool sps_pps_write = false;
  tw_state state;
  seq_state sq_state;

  uint64_t next_timestamp = 0;
  for(size_t kk=0;; kk++) {
    flv::Tag tag;
    uint32_t prev_tag_size;
    sq_state.pos = flv.position();
    if(! flv.parseTag(tag, prev_tag_size)) {
      std::cerr << "parse flv tag failed" << std::endl;
      return 1;
    }
    
    if(flv.eos()) {
      // last 
      sq_state.audio_counter = state.audio_counter % 16;
      sq_state.video_counter = state.video_counter % 16;
      ts_index.write(sq_state.to_char(), sizeof(sq_state));
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
      sq_state.audio_counter = state.audio_counter % 16;
      sq_state.video_counter = state.video_counter % 16;
      ts_index.write(sq_state.to_char(), sizeof(sq_state));

      m3u8 << "#EXTINF:" << duration << std::endl
           << basename(output_prefix.c_str()) << "-" << ts_seq << ".ts" << std::endl 
           << std::endl;

      sps_pps_write = false;      
      switched = false;
      ts_seq++;
      next_timestamp = tag.timestamp + duration * 1000;
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
      
      write_audio(state, tag, buf2, null_out);
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
        // XXX: 既に conf が読み込まれていることが前提
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
        write_video(state, tag, buf, null_out);
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

  // output m3u8 tail
  m3u8 << "#EXT-X-ENDLIST" << std::endl;
  
  return 0;
}
