#include <iostream>
#include <string>
#include <stdlib.h>
#include <inttypes.h>
#include <aux/file_mapped_memory.hh>
#include <h264/avc_decoder_configuration_record.hh>
#include <sstream>
#include <string.h>

#include <flv/parser.hh>
#include <adts/header.hh>
#include <ts/packet.hh>
#include <aux/crc32.hh>
#include <h264/avc_sample.hh>

#include "../bin/ts_write.hh"

using namespace flv2ts;

namespace {
  int getts_impl(const char* flv_path, const char* hls_index_prefix, unsigned sequence, std::ostream& ts_out) {
    const std::string prefix = hls_index_prefix;

    // read configuration
    h264::AVCDecoderConfigurationRecord conf;
    {
      aux::FileMappedMemory conf_mm((prefix + ".avc_conf").c_str());
      if(! conf_mm) {
        std::cerr << "Can't open file(1): " << prefix + ".avc_conf" << std::endl;
        return 0;
      }
      aux::ByteStream conf_in(conf_mm.ptr<uint8_t>(), conf_mm.size());
      if(! conf.parse(conf_in)) {
        std::cerr << "parse AVCDecoderConfigurationRecord failed" << std::endl;
        return 0;
      }
    }
    
    // read ts index
    seq_state start_state;
    seq_state end_state;
    {
      aux::FileMappedMemory index_mm((prefix + ".ts_idx").c_str());
      if(! index_mm) {
        std::cerr << "Can't open file(2): " << prefix + ".ts_idx" << std::endl;
        return 0;
      }
      
      if(sequence < 0 || sequence > index_mm.size() / sizeof(uint32_t)) {
        std::cerr << "out of range: sequence=" << sequence << std::endl;
        return 0;
      }
      
      start_state = *index_mm.ptr<seq_state>((sequence+0) * sizeof(seq_state));
      end_state   = *index_mm.ptr<seq_state>((sequence+1) * sizeof(seq_state));
    }
    
    // flv open
    aux::FileMappedMemory flv_mm(flv_path);
    if(! flv_mm) {
      std::cerr << "Can't open file(3): " << flv_path << " [" << start_state.pos << ".." << end_state.pos << "]" << std::endl;
      return 0;
    }

    flv::Parser flv(flv_mm, start_state.pos, end_state.pos);
    if(! flv) {
      std::cerr << "flv parser initialization failed" << std::endl;
      return 0;
    }
    
    // output ts
    tw_state state;
    state.video_counter = start_state.video_counter;
    state.audio_counter = start_state.audio_counter;
    
    // PAT/PMT
    write_ts_start(ts_out);
    bool video_first = true;
    for(; ! flv.eos();) {
      flv::Tag tag;
      uint32_t prev_tag_size;
      if(! flv.parseTag(tag, prev_tag_size)) {
        std::cerr << "parse flv tag failed" << std::endl;
        return 0;
      }
      
      switch(tag.type) {
      case flv::Tag::TYPE_AUDIO: {
        // audio
        if(tag.audio.sound_format != 10) { // 10=AAC
          std::cerr << "unsupported audio format: " << tag.audio.sound_format << std::endl;
          return 0;
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
          return 0;
        }
        
        switch(tag.video.avc_packet_type) {
        case 0: {
          // AVC sequence header
          break;
        }
        case 1: {
          // AVC NALU
          std::string buf;
          
          // TODO: 既に payload に SPS/PPS が含まれているかのチェック
          if(video_first) {
            to_storage_format_sps_pps(conf, buf);  // payloadに SPS/PPS も含める
            video_first = false;
          }
          
          if(! to_storage_format(conf, tag.video.payload, tag.video.payload_size, buf)) {
            std::cerr << "to_strage_format() failed" << std::endl;
            return 0;
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
          return 0;
        }
        }
      }
        
      default:
        break;
      }
    }
    
    return 1;
  }
}

extern "C" {
  int getts(const char* flv_file, const char* hls_index_prefix, unsigned sequence) {
    return getts_impl(flv_file, hls_index_prefix, sequence, std::cout);
  }

  const char* getts_to_str(const char* flv_file, const char* hls_index_prefix, unsigned sequence, unsigned& size) {
    std::ostringstream ts_out;
    if(getts_impl(flv_file, hls_index_prefix, sequence, ts_out) == 0) {
      size = 0;
      return NULL;
    }
    
    size = ts_out.str().size();
    char* buf = new char[size];
    memcpy(buf, ts_out.str().c_str(), size);
    return buf;
  }
}
