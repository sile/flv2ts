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

const static unsigned PMT_PID = 0x1000;
const static unsigned ES_VIDEO_PID = 0x100;
const static unsigned ES_AUDIO_PID = 0x101;
const static unsigned STREAM_TYPE_VIDEO = 27;
const static unsigned STREAM_TYPE_AUDIO = 15;
const static unsigned VIDEO_STREAM_ID = 224;
const static unsigned AUDIO_STREAM_ID = 192;
const static unsigned PMT_TABLE_ID = 2;
const static double PTS_DTS_OFFSET = 0.1;

static unsigned g_counter = 0; // XXX:
static unsigned g_audio_counter = 0; // XXX:
static uint64_t g_prev_pcr = 0;

void write_ts_pat(std::ostream& out) {
  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = true;
  h.transport_priority = 0;
  h.pid = 0;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 1; // payload unit only
  h.continuity_counter = 0;
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  ts::PAT pat;
  pat.pointer_field = 0;
  pat.table_id = 0;
  pat.section_syntax_indicator = true;
  pat.zero = 0;
  pat.reserved1 = 3;
  pat.transport_stream_id = 1;
  pat.version_number = 0;
  pat.reserved2 = 3;
  pat.current_next_indicator = 1;
  pat.section_number = 0;
  pat.last_section_number = 0;
  
  {
    ts::PMT_MAP_ENTRY entry;
    entry.program_num = 1;
    entry.reserved = 7;
    entry.program_pid = PMT_PID;

    pat.pmt_map.push_back(entry);
  }

  pat.section_length = 9 + pat.pmt_map.size()*4;
  
  {
    size = pat.dump(buf+wrote_size, sizeof(buf)-wrote_size);
    pat.crc32 = aux::chksum_crc32(buf + 5, wrote_size + size - 4 - 5);
  }
    
  size = pat.dump(buf+wrote_size, sizeof(buf)-wrote_size);
  wrote_size += size;

  for(; wrote_size < ts::Packet::SIZE; wrote_size++) {
    buf[wrote_size] = (char)0xFF;
  }

  out.write(buf, wrote_size);
}

void write_ts_pmt(std::ostream& out) {
  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = true;
  h.transport_priority = 0;
  h.pid = PMT_PID;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 1; // payload unit only
  h.continuity_counter = 0;
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  ts::PMT pmt;
  pmt.pointer_field = 0;
  pmt.table_id = PMT_TABLE_ID;
  pmt.section_syntax_indicator = true;
  pmt.zero = 0;
  pmt.reserved1 = 3;
  pmt.program_num = 1;
  pmt.version_number = 0;
  pmt.reserved2 = 3;
  pmt.current_next_indicator = 1;
  pmt.section_number = 0;
  pmt.last_section_number = 0;
  
  pmt.reserved3 = 7;
  pmt.pcr_pid = ES_VIDEO_PID; // PID of general timecode stream, or 0x1FFF
  pmt.reserved4 = 15;
  pmt.program_info_length = 0;

  {
    // video
    ts::STREAM_INFO info;
    info.stream_type    = STREAM_TYPE_VIDEO;
    info.reserved1      = 7;
    info.elementary_pid = ES_VIDEO_PID;
    info.reserved2      = 15;
    info.es_info_length = 0;
    pmt.stream_info_list.push_back(info);

    // audio
    info.stream_type    = STREAM_TYPE_AUDIO;
    info.elementary_pid = ES_AUDIO_PID;
    pmt.stream_info_list.push_back(info);
  }

  pmt.section_length = 13 + pmt.program_info_length + pmt.stream_info_list.size() * 5;
  for(size_t i=0; i < pmt.stream_info_list.size(); i++) {
    pmt.section_length += pmt.stream_info_list[i].es_info_length;
  }
  
  {
    size = pmt.dump(buf+wrote_size, sizeof(buf)-wrote_size);
    pmt.crc32 = aux::chksum_crc32(buf + 5, wrote_size + size - 4 - 5);
  }
    
  size = pmt.dump(buf+wrote_size, sizeof(buf)-wrote_size);
  wrote_size += size;

  for(; wrote_size < ts::Packet::SIZE; wrote_size++) {
    buf[wrote_size] = (char)0xFF;
  }

  out.write(buf, wrote_size);
}

void write_ts_start(std::ostream& out) {
  write_ts_pat(out);
  write_ts_pmt(out);
}

uint64_t sec_to_27MHz(double sec) { // for PCR extention
  return static_cast<uint64_t>(sec * 27000000.0);
}
uint64_t sec_to_90kHz(double sec) {
  return static_cast<uint64_t>(sec * 90000.0);
}

void write_video_first(const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset,
                       size_t stuffing_byte_size=0) {
  const flv::VideoTag& video = tag.video;

  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  // sync-byte
  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  // header
  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = true;
  h.transport_priority = 0;
  h.pid = ES_VIDEO_PID;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 3; // payload and adaptation_field
  h.continuity_counter = (g_counter++) % 16; // XXX:
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // adaptation_field
  ts::AdaptationField af;
  af.discontinuity_indicator = false;
  af.random_access_indicator = (video.frame_type == flv::VideoTag::FRAME_TYPE_KEY);
  af.es_priority_indicator =  false;
  af.pcr_flag = true; // XXX: 毎回書く必要はない (100msに一回)
  af.opcr_flag = false;
  af.splicing_point_flag = false;
  af.transport_private_data_flag = false;
  af.adaptation_field_extension_flag = false;

  uint64_t pcr = sec_to_90kHz(static_cast<double>(tag.timestamp) / 1000.0);
  af.pcr = (pcr << 15) + (0x3F << 9);
  g_prev_pcr = pcr;

  af.adaptation_field_length = 7 + stuffing_byte_size;
  
  size = af.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // pes
  ts::PES pes;
  pes.packet_start_prefix_code = 1;
  pes.stream_id = VIDEO_STREAM_ID;

  const unsigned optional_header_size = 13;
  pes.pes_packet_length = optional_header_size + payload.size(); // XXX: video.payload_size が 2byte を超過する時がある

  ts::OptionalPESHeader& oph = pes.optional_header;
  oph.marker_bits = 2;
  oph.scrambling_control = 0;
  oph.priority = 0;
  oph.data_alignment_indicator = false;
  oph.copyright = false;
  oph.original_or_copy = false;
  oph.pts_indicator = true;
  oph.dts_indicator = true;
  oph.escr_flag = false;
  oph.es_rate_flag = false;
  oph.dsm_trick_mode_flag = false;
  oph.additional_copy_info_flag = false;
  oph.crc_flag = false;
  oph.extension_flag = false;
  oph.pes_header_length = 10;
  oph.dts = sec_to_90kHz(static_cast<double>(tag.timestamp) / 1000.0 + PTS_DTS_OFFSET);
  oph.pts = oph.dts + sec_to_90kHz(static_cast<double>(video.composition_time) / 1000.0);

  size = pes.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;

  // video.paylaod_size < (ts::Packet::SIZE - wrote_size) の場合の処理
  if(payload.size() < (ts::Packet::SIZE - wrote_size)) {
    size_t delta =  (ts::Packet::SIZE - wrote_size) - payload.size();
    // XXX: 暫定
    data_offset = 0;
    g_counter--;
    write_video_first(tag, payload, out, data_offset, delta);
    return;
  }

  out.write(reinterpret_cast<const char*>(buf), wrote_size);

  pes.data_size = ts::Packet::SIZE - wrote_size;
  pes.data = reinterpret_cast<const uint8_t*>(payload.data());
  out.write(reinterpret_cast<const char*>(pes.data), pes.data_size);

  data_offset += pes.data_size;
}

void write_video_rest(const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset) {
  const flv::VideoTag& video = tag.video;

  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  // sync-byte
  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  // header
  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = false;
  h.transport_priority = 0;
  h.pid = ES_VIDEO_PID;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 1; // payload only
  h.continuity_counter = (g_counter++) % 16;
  
  if(payload.size() - data_offset < ( ts::Packet::SIZE - wrote_size - 3)) { // XXX: いろいろ
    h.adaptation_field_exist |= 2;
  }

  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;

  size_t rest_size = ts::Packet::SIZE - wrote_size;
  if(payload.size() - data_offset < rest_size) {
    // adaptation_field
    ts::AdaptationField af;
    af.discontinuity_indicator = false;
    af.random_access_indicator = (video.frame_type == flv::VideoTag::FRAME_TYPE_KEY);
    af.es_priority_indicator =  false;
    af.pcr_flag = false;
    af.opcr_flag = false;
    af.splicing_point_flag = false;
    af.transport_private_data_flag = false;
    af.adaptation_field_extension_flag = false;
    
    af.adaptation_field_length = 1;
    
    if(payload.size() - data_offset < rest_size - 2) { // 2 = adaptation-fieldの最小サイズ
      size_t delta = rest_size - (payload.size() - data_offset) - 2;
      af.adaptation_field_length += delta;
    }

    size = af.dump(buf + wrote_size, sizeof(buf) - wrote_size);
    wrote_size += size;
    rest_size  -= size;
  }
  out.write(reinterpret_cast<const char*>(buf), wrote_size);
  
  // pes: data
  out.write(payload.data()+data_offset, rest_size);
  data_offset += rest_size;
}

void write_video(const flv::Tag& tag, const std::string& payload, std::ostream& out) {
  size_t data_offset=0;
  write_video_first(tag, payload, out, data_offset);
  for(; data_offset < payload.size(); ) {
    write_video_rest(tag, payload, out, data_offset);
  }
}

void write_audio_first(const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset,
                       size_t stuffing_byte_size=0) {
  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  // sync-byte
  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  // header
  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = true;
  h.transport_priority = 0;
  h.pid = ES_AUDIO_PID;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 3; // payload and adaptation_field
  h.continuity_counter = (g_audio_counter++) % 16; // XXX:
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // adaptation_field
  ts::AdaptationField af;
  af.discontinuity_indicator = false;
  af.random_access_indicator = true;
  af.es_priority_indicator =  false;
  af.pcr_flag = false;
  af.opcr_flag = false;
  af.splicing_point_flag = false;
  af.transport_private_data_flag = false;
  af.adaptation_field_extension_flag = false;

  af.adaptation_field_length = 7 + stuffing_byte_size;
  
  size = af.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // pes
  ts::PES pes;
  pes.packet_start_prefix_code = 1;
  pes.stream_id = AUDIO_STREAM_ID;

  const unsigned optional_header_size = 13;
  pes.pes_packet_length = optional_header_size + payload.size(); // XXX: video.payload_size が 2byte を超過する時がある

  ts::OptionalPESHeader& oph = pes.optional_header;
  oph.marker_bits = 2;
  oph.scrambling_control = 0;
  oph.priority = 0;
  oph.data_alignment_indicator = false;
  oph.copyright = false;
  oph.original_or_copy = false;
  oph.pts_indicator = true;
  oph.dts_indicator = true;
  oph.escr_flag = false;
  oph.es_rate_flag = false;
  oph.dsm_trick_mode_flag = false;
  oph.additional_copy_info_flag = false;
  oph.crc_flag = false;
  oph.extension_flag = false;
  oph.pes_header_length = 10;
  oph.dts = sec_to_90kHz(static_cast<double>(tag.timestamp) / 1000.0 + PTS_DTS_OFFSET);
  oph.pts = oph.dts;

  size = pes.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;

  // audio.paylaod_size < (ts::Packet::SIZE - wrote_size) の場合の処理
  if(payload.size() < (ts::Packet::SIZE - wrote_size)) {
    size_t delta =  (ts::Packet::SIZE - wrote_size) - payload.size();
    // XXX: 暫定
    data_offset = 0;
    g_audio_counter--;
    write_audio_first(tag, payload, out, data_offset, delta);
    return;
  }

  out.write(reinterpret_cast<const char*>(buf), wrote_size);

  pes.data_size = ts::Packet::SIZE - wrote_size;
  pes.data = reinterpret_cast<const uint8_t*>(payload.data());
  out.write(reinterpret_cast<const char*>(pes.data), pes.data_size);

  data_offset += pes.data_size;
}

void write_audio_rest(const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset) {
  char buf[256];
  ssize_t size;
  size_t wrote_size = 0;

  // sync-byte
  buf[0] = static_cast<char>(ts::Packet::SYNC_BYTE);
  wrote_size += 1;

  // header
  ts::Header h;
  h.transport_error_indicator = false;
  h.payload_unit_start_indicator = false;
  h.transport_priority = 0;
  h.pid = ES_AUDIO_PID;
  h.scrambling_control = 0;
  h.adaptation_field_exist = 1; // payload only
  h.continuity_counter = (g_audio_counter++) % 16;
  
  if(payload.size() - data_offset < ( ts::Packet::SIZE - wrote_size - 3)) { // XXX: いろいろ
    h.adaptation_field_exist |= 2;
  }

  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;

  size_t rest_size = ts::Packet::SIZE - wrote_size;
  if(payload.size() - data_offset < rest_size) {
    // adaptation_field
    ts::AdaptationField af;
    af.discontinuity_indicator = false;
    af.random_access_indicator = true;
    af.es_priority_indicator =  false;
    af.pcr_flag = false;
    af.opcr_flag = false;
    af.splicing_point_flag = false;
    af.transport_private_data_flag = false;
    af.adaptation_field_extension_flag = false;
    
    af.adaptation_field_length = 1;
    
    if(payload.size() - data_offset < rest_size - 2) { // 2 = adaptation-fieldの最小サイズ
      size_t delta = rest_size - (payload.size() - data_offset) - 2;
      af.adaptation_field_length += delta;
    }

    size = af.dump(buf + wrote_size, sizeof(buf) - wrote_size);
    wrote_size += size;
    rest_size  -= size;
  }
  out.write(reinterpret_cast<const char*>(buf), wrote_size);
  
  // pes: data
  out.write(payload.data()+data_offset, rest_size);
  data_offset += rest_size;
}


// TODO: write_videoを共通化
void write_audio(const flv::Tag& tag, const std::string& payload, std::ostream& out) {
  size_t data_offset=0;
  write_audio_first(tag, payload, out, data_offset);
  for(; data_offset < payload.size(); ) {
    write_audio_rest(tag, payload, out, data_offset);
  }
}


bool to_storage_format_sps_pps(const h264::AVCDecoderConfigurationRecord& conf, std::string& buf) {
  for(uint8_t i=0; i < conf.num_of_sequence_parameter_sets; i++) {
    // start code prefix
    buf += (char)0;
    buf += (char)0;
    buf += (char)1;

    buf += conf.sequence_parameter_set_list[i];
  }
  for(uint8_t i=0; i < conf.num_of_picture_parameter_sets; i++) {
    // start code prefix
    buf += (char)0;
    buf += (char)0;
    buf += (char)1;

    buf += conf.picture_parameter_set_list[i];
  }
  return true;
}

bool to_storage_format(const h264::AVCDecoderConfigurationRecord& conf,
                       const uint8_t* data, size_t data_size, std::string& buf) {
  aux::ByteStream avc_in(data, data_size);
  for(int j=0; ! avc_in.eos(); j++) {
    h264::AVCSample sample;
    if(! sample.parse(avc_in, conf)) {
      std::cerr << "parse AVCSample failed" << std::endl;
      return false;
    }

    // start code prefix
    buf += (char)0;
    buf += (char)0;
    buf += (char)0;
    buf += (char)1;

    buf.append(reinterpret_cast<const char*>(sample.nal_unit), sample.nal_unit_length);
  }

  return true;
}

int main(int argc, char** argv) {
  if(argc != 4) {
    std::cerr << "Usage: flv2ts INPUT_FLV_FILE OUTPUT_DIR DURATION" << std::endl;
    return 1;
  }

  const char* flv_file = argv[1];
  const char* output_dir = argv[2];
  const unsigned duration = atoi(argv[3]);

  flv2ts::flv::Parser flv(flv_file);
  if(! flv) {
    std::cerr << "Can't open file: " << flv_file << std::endl;
    return 1;
  }
  
  std::cout << "[input]" << std::endl
            << "  flv:      " << flv_file << std::endl
            << "  output:   " << output_dir << std::endl
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
  h264::AVCDecoderConfigurationRecord conf;
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

    if(switched) {
      ts_out.close();

      char buf[1024];
      sprintf(buf, "%s/a-%d.ts", output_dir, ts_seq);
      ts_out.open(buf, std::ios::out | std::ios::binary);
      if(! ts_out) {
        std::cerr << "Can't open output file: " << buf << std::endl;
        return 1;
      }
      std::cout << "open: " << buf << std::endl;
      
      switched = false;
      ts_seq++;

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
      // buf2.assign(buf, 7);
      buf2.append(reinterpret_cast<const char*>(tag.audio.payload), tag.audio.payload_size);
      
      write_audio(tag, buf2, ts_out);
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

        // XXX: これがあるとうまくいかない => なくても問題ない？
        // std::string buf;
        // to_storage_format_sps_pps(conf, buf); 
        // write_video(tag, buf, ts_out);
        
        break;
      }
      case 1: {
        // AVC NALU
        std::string buf;
        if(! to_storage_format(conf, tag.video.payload, tag.video.payload_size, buf)) {
          std::cerr << "to_strage_format() failed" << std::endl;
          return 1;
        }
        write_video(tag, buf, ts_out);
        
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
