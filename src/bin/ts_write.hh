#ifndef TS_WRITE_HH
#define TS_WRITE_HH

// XXX: 一時的なファイル => 後でちゃんとしたクラス等に整理

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

// ts_write state
struct tw_state {
  unsigned audio_counter;
  unsigned video_counter;
  bool discontinuity;

  tw_state() 
    : audio_counter(0)
    , video_counter(0)
    , discontinuity(false) {}
};

// XXX:
struct seq_state {
  uint32_t pos;
  uint8_t audio_counter;
  uint8_t video_counter;

  const char* to_char() const { return reinterpret_cast<const char*>(this); }
};

void write_video_first(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset,
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
  h.continuity_counter = state.video_counter++ % 16; // XXX:
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // adaptation_field
  ts::AdaptationField af;
  af.discontinuity_indicator = state.discontinuity;
  af.random_access_indicator = (video.frame_type == flv::VideoTag::FRAME_TYPE_KEY);
  af.es_priority_indicator =  false;
  af.pcr_flag = true; // XXX: 毎回書く必要はない (100msに一回)
  af.opcr_flag = false;
  af.splicing_point_flag = false;
  af.transport_private_data_flag = false;
  af.adaptation_field_extension_flag = false;

  uint64_t pcr = sec_to_90kHz(static_cast<double>(tag.timestamp) / 1000.0);
  af.pcr = (pcr << 15) + (0x3F << 9);

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
    state.video_counter--;
    write_video_first(state, tag, payload, out, data_offset, delta);
    return;
  }

  out.write(reinterpret_cast<const char*>(buf), wrote_size);

  pes.data_size = ts::Packet::SIZE - wrote_size;
  pes.data = reinterpret_cast<const uint8_t*>(payload.data());
  out.write(reinterpret_cast<const char*>(pes.data), pes.data_size);

  data_offset += pes.data_size;
}

void write_video_rest(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset) {
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
  h.continuity_counter = state.video_counter++ % 16;
  
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

void write_video(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out) {
  size_t data_offset=0;
  write_video_first(state, tag, payload, out, data_offset);
  for(; data_offset < payload.size(); ) {
    write_video_rest(state, tag, payload, out, data_offset);
  }
}

void write_audio_first(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset,
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
  h.continuity_counter = state.audio_counter++ % 16; // XXX:
  
  size = h.dump(buf + wrote_size, sizeof(buf) - wrote_size);
  wrote_size += size;
  
  // adaptation_field
  ts::AdaptationField af;
  af.discontinuity_indicator = state.discontinuity;
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
    state.audio_counter--;
    write_audio_first(state, tag, payload, out, data_offset, delta);
    return;
  }

  out.write(reinterpret_cast<const char*>(buf), wrote_size);

  pes.data_size = ts::Packet::SIZE - wrote_size;
  pes.data = reinterpret_cast<const uint8_t*>(payload.data());
  out.write(reinterpret_cast<const char*>(pes.data), pes.data_size);

  data_offset += pes.data_size;
}

void write_audio_rest(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out, size_t& data_offset) {
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
  h.continuity_counter = state.audio_counter++ % 16;
    
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
void write_audio(tw_state& state, const flv::Tag& tag, const std::string& payload, std::ostream& out) {
  size_t data_offset=0;
  write_audio_first(state, tag, payload, out, data_offset);
  for(; data_offset < payload.size(); ) {
    write_audio_rest(state, tag, payload, out, data_offset);
  }
}


bool to_storage_format_sps_pps(const h264::AVCDecoderConfigurationRecord& conf, std::string& buf) {
  for(uint8_t i=0; i < conf.num_of_sequence_parameter_sets; i++) {
    // start code prefix
    //buf += (char)0; // XXX: 不要?
    buf += (char)0;
    buf += (char)0;
    buf += (char)1;

    buf += conf.sequence_parameter_set_list[i];
  }
  for(uint8_t i=0; i < conf.num_of_picture_parameter_sets; i++) {
    // start code prefix
    //buf += (char)0;
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
    //buf += (char)0;
    buf += (char)0;
    buf += (char)0;
    buf += (char)1;

    buf.append(reinterpret_cast<const char*>(sample.nal_unit), sample.nal_unit_length);
  }

  return true;
}


#endif
